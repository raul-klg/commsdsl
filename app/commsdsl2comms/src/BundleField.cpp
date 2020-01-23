//
// Copyright 2018 - 2020 (C). Alex Robenko. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "BundleField.h"

#include <type_traits>
#include <numeric>

#include <boost/algorithm/string.hpp>

#include "Generator.h"
#include "common.h"
#include "IntField.h"

namespace ba = boost::algorithm;

namespace commsdsl2comms
{

namespace
{

const std::string MembersDefTemplate =
    "/// @brief Scope for all the member fields of @ref #^#CLASS_NAME#$# bundle.\n"
    "#^#EXTRA_PREFIX#$#\n"
    "struct #^#CLASS_NAME#$#Members\n"
    "{\n"
    "    #^#MEMBERS_DEFS#$#\n"
    "    /// @brief All members bundled in @b std::tuple.\n"
    "    using All =\n"
    "        std::tuple<\n"
    "           #^#MEMBERS#$#\n"
    "        >;\n"
    "};\n";

const std::string MembersOptionsTemplate =
    "/// @brief Extra options for all the member fields of\n"
    "///     @ref #^#SCOPE#$##^#CLASS_NAME#$# bundle.\n"
    "struct #^#CLASS_NAME#$#Members\n"
    "{\n"
    "    #^#OPTIONS#$#\n"
    "};\n";

const std::string ClassTemplate(
    "#^#MEMBERS_STRUCT_DEF#$#\n"
    "#^#PREFIX#$#"
    "class #^#CLASS_NAME#$# : public\n"
    "    comms::field::Bundle<\n"
    "        #^#PROT_NAMESPACE#$#::field::FieldBase<>,\n"
    "        typename #^#ORIG_CLASS_NAME#$#Members#^#MEMBERS_OPT#$#::All#^#COMMA#$#\n"
    "        #^#FIELD_OPTS#$#\n"
    "    >\n"
    "{\n"
    "    using Base = \n"
    "        comms::field::Bundle<\n"
    "            #^#PROT_NAMESPACE#$#::field::FieldBase<>,\n"
    "            typename #^#ORIG_CLASS_NAME#$#Members#^#MEMBERS_OPT#$#::All#^#COMMA#$#\n"
    "            #^#FIELD_OPTS#$#\n"
    "        >;\n"
    "public:\n"
    "    #^#ACCESS#$#\n"
    "    #^#ALIASES#$#\n"
    "    #^#PUBLIC#$#\n"
    "    #^#NAME#$#\n"
    "    #^#READ#$#\n"
    "    #^#WRITE#$#\n"
    "    #^#LENGTH#$#\n"
    "    #^#VALID#$#\n"
    "    #^#REFRESH#$#\n"
    "#^#PROTECTED#$#\n"
    "#^#PRIVATE#$#\n"
    "};\n"
);

} // namespace

bool BundleField::startsWithValidPropKey() const
{
    if (m_members.empty()) {
        return false;
    }

    auto& first = m_members.front();
    if (first->kind() != commsdsl::Field::Kind::Int) {
        return false;
    }

    auto& keyField = static_cast<const IntField&>(*first);
    if (!keyField.isValidPropKey()) {
        return false;
    }

    // Valid only if there is no non-default read
    return getRead().empty();
}

std::string BundleField::getPropKeyType() const
{
    if (m_members.empty()) {
        return common::emptyString();
    }

    auto& first = m_members.front();
    if (first->kind() != commsdsl::Field::Kind::Int) {
        return common::emptyString();
    }

    auto& keyField = static_cast<const IntField&>(*first);
    return keyField.getPropKeyType();
}

std::string BundleField::getFirstMemberName() const
{
    if (m_members.empty()) {
        return common::emptyString();
    }

    return m_members.front()->name();
}

std::string BundleField::getPropKeyValueStr() const
{
    if (m_members.empty()) {
        return common::emptyString();
    }

    auto& first = m_members.front();
    if (first->kind() != commsdsl::Field::Kind::Int) {
        return common::emptyString();
    }

    auto& keyField = static_cast<const IntField&>(*first);
    return keyField.getPropKeyValueStr();
}

bool BundleField::prepareImpl()
{
    auto obj = bundleFieldDslObj();
    auto members = obj.members();
    m_members.reserve(members.size());
    for (auto& m : members) {
        auto ptr = create(generator(), m);
        if (!ptr) {
            assert(!"should not happen");
            return false;
        }

        ptr->setMemberChild();
        if (!ptr->prepare(obj.sinceVersion())) {
            return false;
        }

        m_members.push_back(std::move(ptr));
    }
    return true;
}

void BundleField::updateIncludesImpl(IncludesList& includes) const
{
    static const IncludesList List = {
        "comms/field/Bundle.h",
        "<tuple>"
    };

    common::mergeIncludes(List, includes);

    for (auto& m : m_members) {
        m->updateIncludes(includes);
    }
}

void BundleField::updateIncludesCommonImpl(IncludesList& includes) const
{
    for (auto& m : m_members) {
        m->updateIncludesCommon(includes);
    }
}

void BundleField::updatePluginIncludesImpl(Field::IncludesList& includes) const
{
    for (auto& m : m_members) {
        m->updatePluginIncludes(includes);
    }
}

std::size_t BundleField::minLengthImpl() const
{
    return
        std::accumulate(
            m_members.begin(), m_members.end(), std::size_t(0),
            [](std::size_t soFar, auto& m)
            {
                return soFar + m->minLength();
            });
}

std::size_t BundleField::maxLengthImpl() const
{
    static const std::size_t MaxLen =
        std::numeric_limits<std::size_t>::max();

    return
        std::accumulate(
            m_members.begin(), m_members.end(), std::size_t(0),
            [](std::size_t soFar, auto& m)
            {
                if (soFar == MaxLen) {
                    return MaxLen;
                }

                auto fLen = m->maxLength();
                if ((MaxLen - soFar) <= fLen) {
                    return MaxLen;
                }

                return soFar + fLen;
            });
}

std::string BundleField::getClassDefinitionImpl(
    const std::string& scope,
    const std::string& className) const
{
    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("PREFIX", getClassPrefix(className)));
    replacements.insert(std::make_pair("CLASS_NAME", className));
    replacements.insert(std::make_pair("ORIG_CLASS_NAME", common::nameToClassCopy(name())));
    replacements.insert(std::make_pair("PROT_NAMESPACE", generator().mainNamespace()));
    replacements.insert(std::make_pair("FIELD_OPTS", getFieldOpts(scope)));
    replacements.insert(std::make_pair("NAME", getNameCommonWrapFunc(adjustScopeWithNamespace(scope))));
    replacements.insert(std::make_pair("READ", getRead()));
    replacements.insert(std::make_pair("WRITE", getCustomWrite()));
    replacements.insert(std::make_pair("LENGTH", getCustomLength()));
    replacements.insert(std::make_pair("VALID", getCustomValid()));
    replacements.insert(std::make_pair("REFRESH", getRefresh()));
    replacements.insert(std::make_pair("MEMBERS_STRUCT_DEF", getMembersDef(scope)));
    replacements.insert(std::make_pair("ACCESS", getAccess()));
    replacements.insert(std::make_pair("ALIASES", getAliases()));
    replacements.insert(std::make_pair("PRIVATE", getPrivate()));
    replacements.insert(std::make_pair("PUBLIC", getExtraPublic()));
    replacements.insert(std::make_pair("PROTECTED", getFullProtected()));
    if (!replacements["FIELD_OPTS"].empty()) {
        replacements["COMMA"] = ',';
    }

    if (!externalRef().empty()) {
        replacements.insert(std::make_pair("MEMBERS_OPT", "<TOpt>"));
    }

    return common::processTemplate(ClassTemplate, replacements);
}

std::string BundleField::getExtraDefaultOptionsImpl(const std::string& scope) const
{
    return getExtraOptions(scope, &Field::getDefaultOptions);
}

std::string BundleField::getExtraBareMetalDefaultOptionsImpl(const std::string& scope) const
{
    return getExtraOptions(scope, &Field::getBareMetalDefaultOptions);
}

std::string BundleField::getPluginAnonNamespaceImpl(
    const std::string& scope,
    bool forcedSerialisedHidden,
    bool serHiddenParam) const
{
    auto fullScope = scope + common::nameToClassCopy(name()) + common::membersSuffixStr();
    if (!externalRef().empty()) {
        fullScope += "<>";
    }
    fullScope += "::";

    common::StringsList props;
    for (auto& f : m_members) {
        props.push_back(f->getPluginCreatePropsFunc(fullScope, forcedSerialisedHidden, serHiddenParam));
    }

    static const std::string Templ =
        "struct #^#CLASS_NAME#$#Members\n"
        "{\n"
        "    #^#PROPS#$#\n"
        "};\n";

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("CLASS_NAME", common::nameToClassCopy(name())));
    replacements.insert(std::make_pair("PROPS", common::listToString(props, "\n", common::emptyString())));
    return common::processTemplate(Templ, replacements);
}

std::string BundleField::getPluginPropertiesImpl(bool serHiddenParam) const
{
    common::StringsList props;
    props.reserve(m_members.size());
    auto prefix =
        common::nameToClassCopy(name()) + common::membersSuffixStr() +
        "::createProps_";
    for (auto& f : m_members) {
        auto str = ".add(" + prefix + common::nameToAccessCopy(f->name()) + "(";
        if (serHiddenParam) {
            str += common::serHiddenStr();
        }
        str += "))";
        props.push_back(std::move(str));
    }

    return common::listToString(props, "\n", common::emptyString());
}

void BundleField::setForcedPseudoImpl()
{
    for (auto& m : m_members) {
        m->setForcedPseudo();
    }
}

void BundleField::setForcedNoOptionsConfigImpl()
{
    for (auto& m : m_members) {
        m->setForcedNoOptionsConfig();
    }
}

bool BundleField::isVersionDependentImpl() const
{
    return
        std::any_of(
            m_members.begin(), m_members.end(),
            [](auto& m)
            {
                return m->isVersionDependent();
            });
}

std::string BundleField::getCommonDefinitionImpl(const std::string& fullScope) const
{
    common::StringsList defs;
    auto updatedScope =
        fullScope + common::membersSuffixStr() + "::";
    for (auto& m : m_members) {
        auto str = m->getCommonDefinition(updatedScope);
        if (!str.empty()) {
            defs.emplace_back(std::move(str));
        }
    }

    std::string membersCommon;
    if (!defs.empty()) {
        static const std::string Templ =
            "/// @brief Scope for all the common definitions of the member fields of\n"
            "///     @ref #^#SCOPE#$# field.\n"
            "struct #^#CLASS_NAME#$#MembersCommon\n"
            "{\n"
            "    #^#DEFS#$#\n"
            "};\n";

        common::ReplacementMap repl;
        repl.insert(std::make_pair("CLASS_NAME", common::nameToClassCopy(name())));
        repl.insert(std::make_pair("SCOPE", fullScope));
        repl.insert(std::make_pair("DEFS", common::listToString(defs, "\n", common::emptyString())));
        membersCommon = common::processTemplate(Templ, repl);
    }

    static const std::string Templ =
        "#^#COMMON#$#\n"
        "/// @brief Scope for all the common definitions of the\n"
        "///     @ref #^#SCOPE#$# field.\n"
        "struct #^#CLASS_NAME#$#Common\n"
        "{\n"
        "    #^#NAME_FUNC#$#\n"
        "};\n\n";

    common::ReplacementMap repl;
    repl.insert(std::make_pair("COMMON", std::move(membersCommon)));
    repl.insert(std::make_pair("CLASS_NAME", common::nameToClassCopy(name())));
    repl.insert(std::make_pair("SCOPE", fullScope));
    repl.insert(std::make_pair("NAME_FUNC", getCommonNameFunc(fullScope)));
    return common::processTemplate(Templ, repl);
}

std::string BundleField::getExtraRefToCommonDefinitionImpl(const std::string& fullScope) const
{
    static const std::string Templ =
        "/// @brief Common types and functions for members of\n"
        "///     @ref #^#SCOPE#$# field.\n"
        "using #^#CLASS_NAME#$#MembersCommon = #^#COMMON_SCOPE#$#MembersCommon;\n\n";

    auto commonScope = scopeForCommon(generator().scopeForField(externalRef(), true, true));
    std::string className = classNameFromFullScope(fullScope);

    common::ReplacementMap repl;
    repl.insert(std::make_pair("SCOPE", fullScope));
    repl.insert(std::make_pair("CLASS_NAME", std::move(className)));
    repl.insert(std::make_pair("COMMON_SCOPE", std::move(commonScope)));

    return common::processTemplate(Templ, repl);
}

bool BundleField::verifyAliasImpl(const std::string& fieldName) const
{
    assert(!fieldName.empty());
    auto dotPos = fieldName.find('.');
    std::string firstFieldName(fieldName, 0, dotPos);
    auto iter =
        std::find_if(
            m_members.begin(), m_members.end(),
            [&firstFieldName](auto& f)
            {
                return firstFieldName == f->name();
            });

    if (iter == m_members.end()) {
        return false;
    }

    if (dotPos == std::string::npos) {
        return true;
    }

    std::string restFieldName(fieldName, dotPos + 1);
    return (*iter)->verifyAlias(restFieldName);
}

std::string BundleField::getFieldOpts(const std::string& scope) const
{
    StringsList options;

    updateExtraOptions(scope, options);

    bool membersHaveCustomReadRefresh =
        std::any_of(
            m_members.begin(), m_members.end(),
            [](auto& m) {
                assert(m);
                return m->hasCustomReadRefresh();
            });

    if (membersHaveCustomReadRefresh) {
        common::addToList("comms::option::def::HasCustomRead", options);
        common::addToList("comms::option::def::HasCustomRefresh", options);
    }

    auto lengthFieldIter = 
         std::find_if(
            m_members.begin(), m_members.end(),
            [](auto& m) {
                assert(m);
                return m->semanticType() == commsdsl::Field::SemanticType::Length;
            });   

    if (lengthFieldIter != m_members.end()) {
        auto idx = static_cast<unsigned>(std::distance(m_members.begin(), lengthFieldIter));
        auto optStr = "comms::option::def::RemLengthMemberField<" + common::numToString(idx) + '>';
        common::addToList(optStr, options);
    }

    return common::listToString(options, ",\n", common::emptyString());
}

std::string BundleField::getMembersDef(const std::string& scope) const
{
    auto className = common::nameToClassCopy(name());
    std::string memberScope;
    if (!scope.empty()) {
        memberScope = scope + className + common::membersSuffixStr() + "::";
    }
    StringsList membersDefs;
    StringsList membersNames;

    membersDefs.reserve(m_members.size());
    membersNames.reserve(m_members.size());
    for (auto& m : m_members) {
        membersDefs.push_back(m->getClassDefinition(memberScope));
        membersNames.push_back(common::nameToClassCopy(m->name()));
    }

    std::string prefix;
    if (!externalRef().empty()) {
        prefix += "/// @tparam TOpt Protocol options.\n";
        prefix += "template <typename TOpt = " + generator().scopeForOptions(common::defaultOptionsStr(), true, true) + ">";
    }

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("CLASS_NAME", className));
    replacements.insert(std::make_pair("EXTRA_PREFIX", std::move(prefix)));
    replacements.insert(std::make_pair("MEMBERS_DEFS", common::listToString(membersDefs, "\n", common::emptyString())));
    replacements.insert(std::make_pair("MEMBERS", common::listToString(membersNames, ",\n", common::emptyString())));
    return common::processTemplate(MembersDefTemplate, replacements);

}

std::string BundleField::getAccess() const
{
    static const std::string Templ =
        "/// @brief Allow access to internal fields.\n"
        "/// @details See definition of @b COMMS_FIELD_MEMBERS_NAMES macro\n"
        "///     related to @b comms::field::Bundle class from COMMS library\n"
        "///     for details.\n"
        "///\n"
        "///     The generated access functions are:\n"
        "#^#ACCESS_DOC#$#\n"
        "COMMS_FIELD_MEMBERS_NAMES(\n"
        "    #^#NAMES#$#\n"
        ");\n";

    StringsList accessDocList;
    StringsList namesList;
    accessDocList.reserve(m_members.size());
    namesList.reserve(m_members.size());

    auto scope = common::nameToClassCopy(name()) + common::membersSuffixStr() + "::";
    for (auto& m : m_members) {
        namesList.push_back(common::nameToAccessCopy(m->name()));
        std::string accessStr =
            "///     @li @b Field_" + namesList.back() +
            " @b field_" + namesList.back() +
            "() -\n"
            "///         for " + scope +
            common::nameToClassCopy(m->name()) + " member field.";
        accessDocList.push_back(std::move(accessStr));
    }

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("ACCESS_DOC", common::listToString(accessDocList, "\n", common::emptyString())));
    replacements.insert(std::make_pair("NAMES", common::listToString(namesList, ",\n", common::emptyString())));
    return common::processTemplate(Templ, replacements);
}

std::string BundleField::getAliases() const
{
    auto obj = bundleFieldDslObj();
    auto aliases = obj.aliases();
    if (aliases.empty()) {
        return common::emptyString();
    }

    common::StringsList result;
    for (auto& a : aliases) {
        auto& fieldName = a.fieldName();
        assert(!fieldName.empty());

        auto dotPos = fieldName.find('.');
        std::string firstFieldName(fieldName, 0, dotPos);
        auto iter =
            std::find_if(
                m_members.begin(), m_members.end(),
                [&firstFieldName](auto& f)
                {
                    return firstFieldName == f->name();
                });

        if (iter == m_members.end()) {
            continue;
        }

        std::string restFieldName;
        if (dotPos != std::string::npos) {
            restFieldName.assign(fieldName, dotPos + 1, fieldName.size());
        }

        if (!restFieldName.empty() && (!(*iter)->verifyAlias(restFieldName))) {
            continue;
        }

        static const std::string Templ =
            "/// @brief Alias to a member field.\n"
            "/// @details\n"
            "#^#ALIAS_DESC#$#\n"
            "///     Generates field access alias function(s):\n"
            "///     @b field_#^#ALIAS_NAME#$#() -> <b>#^#ALIASED_FIELD_DOC#$#</b>\n"
            "COMMS_FIELD_ALIAS(#^#ALIAS_NAME#$#, #^#ALIASED_FIELD#$#);\n";

        std::vector<std::string> aliasedFields;
        ba::split(aliasedFields, fieldName, ba::is_any_of("."));
        std::string aliasedFieldDocStr;
        std::string aliasedFieldStr;
        for (auto& f : aliasedFields) {
            common::nameToAccess(f);

            if (!aliasedFieldDocStr.empty()) {
                aliasedFieldDocStr += '.';
            }
            aliasedFieldDocStr += "field_" + f + "()";

            if (!aliasedFieldStr.empty()) {
                aliasedFieldStr += ", ";
            }

            aliasedFieldStr += f;
        }

        auto desc = common::makeDoxygenMultilineCopy(a.description());
        if (!desc.empty()) {
            desc = common::doxygenPrefixStr() + common::indentStr() + desc + " @n";
        }

        common::ReplacementMap repl;
        repl.insert(std::make_pair("ALIAS_NAME", common::nameToAccessCopy(a.name())));
        repl.insert(std::make_pair("ALIASED_FIELD_DOC", std::move(aliasedFieldDocStr)));
        repl.insert(std::make_pair("ALIASED_FIELD", std::move(aliasedFieldStr)));
        repl.insert(std::make_pair("ALIAS_DESC", std::move(desc)));
        result.push_back(common::processTemplate(Templ, repl));
    }

    if (result.empty()) {
        return common::emptyString();
    }

    return common::listToString(result, "\n", common::emptyString());
}

std::string BundleField::getRead() const
{
    auto customRead = getCustomRead();
    if (!customRead.empty()) {
        return customRead;
    }

    return getReadForFields(m_members);
}

std::string BundleField::getRefresh() const
{
    auto customRefresh = getCustomRefresh();
    if (!customRefresh.empty()) {
        return customRefresh;
    }

    return getPublicRefreshForFields(m_members, false);
}

std::string BundleField::getPrivate() const
{
    auto str = getExtraPrivate();
    auto refreshStr = getPrivateRefreshForFields(m_members);

    if ((!str.empty()) && (refreshStr.empty())) {
        str += '\n';
    }

    str += refreshStr;
    if (str.empty()) {
        return str;
    }

    common::insertIndent(str);
    static const std::string Prefix("private:\n");
    return Prefix + str; 
}

std::string BundleField::getExtraOptions(
    const std::string& scope,
    GetExtraOptionsFunc func) const
{
    std::string memberScope = scope + common::nameToClassCopy(name()) + common::membersSuffixStr() + "::";
    StringsList options;
    options.reserve(m_members.size());
    for (auto& m : m_members) {
        assert(m);
        auto opt = (m.get()->*func)(memberScope);
        if (!opt.empty()) {
            options.push_back(std::move(opt));
        }
    }

    if (options.empty()) {
        return common::emptyString();
    }

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("CLASS_NAME", common::nameToClassCopy(name())));
    replacements.insert(std::make_pair("SCOPE", scope));
    replacements.insert(std::make_pair("OPTIONS", common::listToString(options, "\n", common::emptyString())));
    return common::processTemplate(MembersOptionsTemplate, replacements);
}


} // namespace commsdsl2comms
