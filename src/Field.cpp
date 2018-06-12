#include "Field.h"

#include <functional>
#include <type_traits>
#include <cassert>
#include <algorithm>
#include <fstream>

#include <boost/algorithm/string.hpp>

#include "Generator.h"
#include "IntField.h"
#include "RefField.h"
#include "common.h"

namespace ba = boost::algorithm;

namespace commsdsl2comms
{

namespace
{

const std::string FileTemplate(
    "/// @file\n"
    "/// @brief Contains definition of <b>\"#^#FIELD_NAME#$#\"<\\b> field.\n"
    "\n"
    "#pragma once\n"
    "\n"
    "#^#INCLUDES#$#\n"
    "#^#BEGIN_NAMESPACE#$#\n"
    "#^#CLASS_DEF#$#\n"
    "#^#END_NAMESPACE#$#\n"
);

Field::IncludesList prepareCommonIncludes(const Generator& generator)
{
    Field::IncludesList list = {
        "comms/options.h",
        generator.mainNamespace() + '/' + common::fieldBaseStr() + common::headerSuffix(),
    };

    return list;
}

} // namespace

void Field::updateIncludes(Field::IncludesList& includes) const
{
    static const IncludesList CommonIncludes = prepareCommonIncludes(m_generator);
    common::mergeIncludes(CommonIncludes, includes);
    common::mergeIncludes(extraIncludesImpl(), includes);
    if (!m_externalRef.empty()) {
        auto inc =
            m_generator.mainNamespace() + '/' + common::defaultOptionsStr() + common::headerSuffix();
        common::mergeInclude(inc, includes);
    }
}

bool Field::doesExist() const
{
    return
        m_generator.doesElementExist(
            m_dslObj.sinceVersion(),
            m_dslObj.deprecatedSince(),
                m_dslObj.sinceVersion());
}

bool Field::prepare()
{
    m_externalRef = m_dslObj.externalRef();
    return prepareImpl();
}

std::string Field::getClassDefinition(const std::string& scope) const
{
    std::string str = "/// @brief Definition of <b>\"";
    str += getDisplayName();
    str += "\"<\\b> field.\n";

    auto& desc = m_dslObj.description();
    if (!desc.empty()) {
        str += "/// @details\n";
        auto multiDesc = common::makeMultiline(desc);
        common::insertIndent(multiDesc);
        auto& doxygenPrefix = common::doxygenPrefixStr();
        multiDesc.insert(multiDesc.begin(), doxygenPrefix.begin(), doxygenPrefix.end());
        ba::replace_all(multiDesc, "\n", "\n" + doxygenPrefix);
        str += multiDesc;
        str += '\n';
    }

    if (!m_externalRef.empty()) {
        str += "/// @tparam TOpt Protocol options.\n";
        str += "/// @tparam TExtraOpts Extra options.\n";
        str += "template <typename TOpt = ";
        str += m_generator.mainNamespace();
        str += "::";
        str += common::defaultOptionsStr();
        str += ", typename... TExtraOpts>\n";
    }

    str += getClassDefinitionImpl(scope);
    return str;
}

Field::Ptr Field::create(Generator& generator, commsdsl::Field field)
{
    using CreateFunc = std::function<Ptr (Generator& generator, commsdsl::Field)>;
    static const CreateFunc Map[] = {
        /* Int */ [](Generator& g, commsdsl::Field f) { return createIntField(g, f); },
        /* Enum */ [](Generator&, commsdsl::Field) { return Ptr(); },
        /* Set */ [](Generator&, commsdsl::Field) { return Ptr(); },
        /* Float */ [](Generator&, commsdsl::Field) { return Ptr(); },
        /* Bitfield */ [](Generator&, commsdsl::Field) { return Ptr(); },
        /* Bundle */ [](Generator&, commsdsl::Field) { return Ptr(); },
        /* String */ [](Generator&, commsdsl::Field) { return Ptr(); },
        /* Data */ [](Generator&, commsdsl::Field) { return Ptr(); },
        /* List */ [](Generator&, commsdsl::Field) { return Ptr(); },
        /* Ref */ [](Generator& g, commsdsl::Field f) { return createRefField(g, f); },
        /* Optional */ [](Generator&, commsdsl::Field) { return Ptr(); },
    };

    static const std::size_t MapSize = std::extent<decltype(Map)>::value;
    static_assert(MapSize == (unsigned)commsdsl::Field::Kind::NumOfValues, "Invalid map");

    auto idx = static_cast<std::size_t>(field.kind());
    if (MapSize <= idx) {
        assert(!"Unexpected field kind");
        return Ptr();
    }

    return Map[idx](generator, field);
}

std::string Field::getDefaultOptions() const
{
    return "using " + common::nameToClassCopy(name()) + " = comms::option::EmptyOption;\n";
}

bool Field::writeProtocolDefinition() const
{
    auto startInfo = m_generator.startFieldProtocolWrite(m_externalRef);
    auto& filePath = startInfo.first;
    if (filePath.empty()) {
        return true;
    }

    assert(!m_externalRef.empty());
    IncludesList includes;
    updateIncludes(includes);
    auto incStr = common::includesToStatements(includes);

    auto namespaces = m_generator.namespacesForField(m_externalRef);

    // TODO: modifile class name

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("INCLUDES", std::move(incStr)));
    replacements.insert(std::make_pair("BEGIN_NAMESPACE", std::move(namespaces.first)));
    replacements.insert(std::make_pair("END_NAMESPACE", std::move(namespaces.second)));
    replacements.insert(std::make_pair("CLASS_DEF", getClassDefinition("TOpt::" + m_generator.scopeForField(m_externalRef))));

    std::string str = common::processTemplate(FileTemplate, replacements);

    std::ofstream stream(filePath);
    if (!stream) {
        m_generator.logger().error("Failed to open \"" + filePath + "\" for writing.");
        return false;
    }
    stream << str;

    if (!stream.good()) {
        m_generator.logger().error("Failed to write \"" + filePath + "\".");
        return false;
    }

    return true;
}

const std::string& Field::getDisplayName() const
{
    auto* displayName = &m_dslObj.displayName();
    if (displayName->empty()) {
        displayName = &m_dslObj.name();
    }
    return *displayName;
}

bool Field::prepareImpl()
{
    return true;
}

const Field::IncludesList& Field::extraIncludesImpl() const
{
    static const IncludesList List;
    return List;
}


std::string Field::getNameFunc() const
{
    return
        "/// @brief Name of the field.\n"
        "static const char* name()\n"
        "{\n"
        "    return \"" + getDisplayName() + "\";\n"
                                             "}\n";
}

void Field::updateExtraOptions(const std::string& scope, common::StringsList& options) const
{
    if (!m_externalRef.empty()) {
        options.push_back("TExtraOpts...");
    }

    if (!scope.empty()) {
        options.push_back("typename " + scope + common::nameToClassCopy(name()));
    }
}

} // namespace commsdsl2comms
