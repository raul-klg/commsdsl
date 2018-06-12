#include "Interface.h"

#include <cassert>
#include <fstream>
#include <map>
#include <algorithm>
#include <iterator>
#include <numeric>

#include <boost/algorithm/string.hpp>

#include "Generator.h"
#include "common.h"

namespace ba = boost::algorithm;

namespace commsdsl2comms
{

namespace
{

const std::string AliasTemplate(
    "/// @file\n"
    "/// @brief Contains definition of <b>\"#^#CLASS_NAME#$#\"<\\b> interface class.\n"
    "\n"
    "#pragma once\n"
    "\n"
    "#^#INCLUDES#$#\n"
    "#^#BEGIN_NAMESPACE#$#\n"
    "/// @brief Definition of <b>\"#^#CLASS_NAME#$#\"<\\b> common interface class.\n"
    "#^#DOC_DETAILS#$#\n"
    "/// @tparam TOpt Interface definition options\n"
    "/// @headerfile #^#HEADERFILE#$#\n"
    "template <typename... TOpt>\n"
    "using #^#CLASS_NAME#$# =\n"
    "    comms::Message<\n"
    "        TOpt...,\n"
    "        #^#ENDIAN#$#,\n"
    "        comms::option::MsgIdType<#^#PROT_NAMESPACE#$#::MsgId>\n"
    "    >;\n\n"
    "#^#END_NAMESPACE#$#\n"
);

const std::string ClassTemplate(
    "/// @file\n"
    "/// @brief Contains definition of <b>\"#^#CLASS_NAME#$#\"<\\b> interface class.\n"
    "\n"
    "#pragma once\n"
    "\n"
    "#^#INCLUDES#$#\n"
    "#^#BEGIN_NAMESPACE#$#\n"
    "/// @brief Extra transport fields of @ref #^#CLASS_NAME#$# interface class.\n"
    "/// @see @ref #^#CLASS_NAME#$#\n"
    "/// @headerfile #^#HEADERFILE#$#\n"
    "struct #^#CLASS_NAME#$#Fields\n"
    "{\n"
    "    #^#FIELDS_DEF#$#\n"
    "    /// @brief All the fields bundled in std::tuple.\n"
    "    using All = std::tuple<\n"
    "        #^#FIELDS_LIST#$#\n"
    "    >;\n"
    "};\n\n"
    "/// @brief Definition of <b>\"#^#CLASS_NAME#$#\"<\\b> common interface class.\n"
    "#^#DOC_DETAILS#$#\n"
    "/// @tparam TOpt Interface definition options\n"
    "/// @headerfile #^#HEADERFILE#$#\n"
    "template <typename... TOpt>\n"
    "class #^#CLASS_NAME#$# : public\n"
    "    comms::Message<\n"
    "        TOpt...,\n"
    "        #^#ENDIAN#$#,\n"
    "        comms::option::MsgIdType<#^#PROT_NAMESPACE#$#::MsgId>,\n"
    "        #^#FIELDS_OPTIONS#$#\n"
    "    >\n"
    "{\n"
    "    using Base =\n"
    "        comms::Message<\n"
    "            TOpt...,\n"
    "            #^#ENDIAN#$#,\n"
    "            comms::option::MsgIdType<#^#PROT_NAMESPACE#$#::MsgId>,\n"
    "            #^#FIELDS_OPTIONS#$#\n"
    "        >;\n"
    "public:\n"
    "    /// @brief Allow access to extra transport fields.\n"
    "    /// @details See definition of @b COMMS_MSG_TRANSPORT_FIELDS_ACCESS macro\n"
    "    ///     related to @b comms::Message class from COMMS library\n"
    "    ///     for details.\n"
    "    ///\n"
    "    ///     The generated functions are:\n"
    "    #^#ACCESS_FUNCS_DOC#$#\n"
    "    COMMS_MSG_TRANSPORT_FIELDS_ACCESS(\n"
    "       #^#FIELDS_ACCESS_LIST#$#\n"
    "    );\n"
    "};\n\n"
    "#^#END_NAMESPACE#$#\n"
);

} // namespace

bool Interface::prepare()
{
    if (!m_dslObj.valid()) {
        return true;
    }


    m_externalRef = m_dslObj.externalRef();
    if (m_externalRef.empty()) {
        m_generator.logger().log(commsdsl::ErrorLevel_Error, "Unknown external reference for message: " + m_dslObj.name());
        return false;
    }

    auto dslFields = m_dslObj.fields();
    m_fields.reserve(dslFields.size());
    for (auto& f : dslFields) {
        auto ptr = Field::create(m_generator, f);
        assert(ptr);
        if (!ptr->doesExist()) {
            continue;
        }

        if (!ptr->prepare()) {
            return false;
        }
        m_fields.push_back(std::move(ptr));
    }

    return true;
}

bool Interface::write()
{
    // TODO: write plugin
    return writeProtocol();
}

bool Interface::writeProtocol()
{
    auto names =
        m_generator.startInterfaceProtocolWrite(m_externalRef);
    auto& filePath = names.first;
    auto& className = names.second;

    if (filePath.empty()) {
        // Skipping generation
        return true;
    }

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("CLASS_NAME", className));
    replacements.insert(std::make_pair("PROT_NAMESPACE", m_generator.mainNamespace()));
    replacements.insert(std::make_pair("DOC_DETAILS", getDescription()));
    replacements.insert(std::make_pair("ENDIAN", common::dslEndianToOpt(m_generator.schemaEndian())));
    replacements.insert(std::make_pair("INCLUDES", getIncludes()));
    replacements.insert(std::make_pair("HEADERFILE", m_generator.headerfileForInterface(m_externalRef)));

    auto namespaces = m_generator.namespacesForInterface(m_externalRef);
    replacements.insert(std::make_pair("BEGIN_NAMESPACE", std::move(namespaces.first)));
    replacements.insert(std::make_pair("END_NAMESPACE", std::move(namespaces.second)));

    auto* templ = &AliasTemplate;
    if (!m_fields.empty()) {
        templ = &ClassTemplate;
        replacements.insert(std::make_pair("FIELDS_LIST", getFieldsClassesList()));
        replacements.insert(std::make_pair("FIELDS_ACCESS_LIST", getFieldsAccessList()));
        replacements.insert(std::make_pair("FIELDS_OPTIONS", getFieldsOpts()));
        replacements.insert(std::make_pair("ACCESS_FUNCS_DOC", getFieldsAccessDoc()));
        replacements.insert(std::make_pair("FIELDS_DEF", getFieldsDef()));
    }
    auto str = common::processTemplate(*templ, replacements);

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

std::string Interface::getDescription() const
{
    if (!m_dslObj.valid()) {
        return common::emptyString();
    }

    auto desc = common::makeMultiline(m_dslObj.description());
    if (!desc.empty()) {
        static const std::string DocPrefix("/// @details ");
        desc.insert(desc.begin(), DocPrefix.begin(), DocPrefix.end());
        static const std::string DocNewLineRepl("\n" + common::doxygenPrefixStr() + "    ");
        ba::replace_all(desc, "\n", DocNewLineRepl);
    }
    return desc;
}

std::string Interface::getFieldsClassesList() const
{
    std::string result;
    for (auto& f : m_fields) {
        if (!result.empty()) {
            result += ",\n";
        }
        result += common::nameToClassCopy(f->name());
    }
    return result;
}

std::string Interface::getFieldsAccessList() const
{
    std::string result;
    for (auto& f : m_fields) {
        if (!result.empty()) {
            result += ",\n";
        }
        result += common::nameToAccessCopy(f->name());
    }
    return result;
}

std::string Interface::getIncludes() const
{
    common::StringsList includes;
    for (auto& f : m_fields) {
        f->updateIncludes(includes);
    }

    if (!m_fields.empty()) {
        common::mergeInclude("<tuple>", includes);
    }

    static const common::StringsList InterfaceIncludes = {
        "comms/Message.h",
        "comms/options.h",
        m_generator.mainNamespace() + '/' + common::msgIdEnuNameStr() + common::headerSuffix()
    };

    common::mergeIncludes(InterfaceIncludes, includes);
    return common::includesToStatements(includes);
}

std::string Interface::getFieldsAccessDoc() const
{
    if (m_fields.empty()) {
        return common::emptyString();
    }

    std::string result;
    for (auto& f : m_fields) {
        if (!result.empty()) {
            result += '\n';
        }
        result += common::doxygenPrefixStr();
        result += common::indentStr();
        result += "@li @b transportField_";
        result += common::nameToAccessCopy(f->name());
        result += "() for @ref ";
        result += common::nameToClassCopy(m_dslObj.name());
        result += "Fields::";
        result += common::nameToClassCopy(f->name());
        result += " field.";
    }

    return result;
}

std::string Interface::getFieldsDef() const
{
    std::string result;

    for (auto& f : m_fields) {
        result += f->getClassDefinition(common::emptyString());
        if (&f != &m_fields.back()) {
            result += '\n';
        }
    }
    return result;
}

std::string Interface::getFieldsOpts() const
{
    std::string result =
        "comms::option::ExtraTransportFields<" +
        common::nameToClassCopy(m_dslObj.name()) + 
        common::fieldsSuffixStr() + 
        "::All>";

    auto iter =
        std::find_if(
            m_fields.begin(), m_fields.end(),
            [](auto& f)
            {
                return f->semanticType() == commsdsl::Field::SemanticType::Version;
            });

    if (iter != m_fields.end()) {
        result += ",\n";
        result += "comms::option::VersionInExtraTransportFields<";
        result += common::numToString(static_cast<std::size_t>(std::distance(m_fields.begin(), iter)));
        result += ">";
    }
    return result;
}

}
