//
// Copyright 2018 (C). Alex Robenko. All rights reserved.
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

#include "VariantField.h"

#include <type_traits>
#include <numeric>

#include <boost/algorithm/string.hpp>

#include "Generator.h"
#include "common.h"

namespace ba = boost::algorithm;

namespace commsdsl2comms
{

namespace
{

const std::string MembersDefTemplate =
    "/// @brief Scope for all the member fields of @ref #^#CLASS_NAME#$# bitfield.\n"
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
    "/// @brief Extra options for all the member fields of @ref #^#SCOPE#$##^#CLASS_NAME#$# bitfield.\n"
    "struct #^#CLASS_NAME#$#Members\n"
    "{\n"
    "    #^#OPTIONS#$#\n"
    "};\n";

const std::string ClassTemplate(
    "#^#MEMBERS_STRUCT_DEF#$#\n"
    "#^#PREFIX#$#"
    "class #^#CLASS_NAME#$# : public\n"
    "    comms::field::Variant<\n"
    "        #^#PROT_NAMESPACE#$#::field::FieldBase<>,\n"
    "        typename #^#ORIG_CLASS_NAME#$#Members#^#MEMBERS_OPT#$#::All#^#COMMA#$#\n"
    "        #^#FIELD_OPTS#$#\n"
    "    >\n"
    "{\n"
    "    using Base = \n"
    "        comms::field::Variant<\n"
    "            #^#PROT_NAMESPACE#$#::field::FieldBase<>,\n"
    "            typename #^#ORIG_CLASS_NAME#$#Members#^#MEMBERS_OPT#$#::All#^#COMMA#$#\n"
    "            #^#FIELD_OPTS#$#\n"
    "        >;\n"
    "public:\n"
    "    #^#ACCESS#$#\n"
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

bool VariantField::prepareImpl()
{
    auto obj = variantFieldDslObj();
    auto members = obj.members();
    m_members.reserve(members.size());
    for (auto& m : members) {
        auto ptr = create(generator(), m);
        if (!ptr) {
            assert(!"should not happen");
            return false;
        }

        if (!ptr->prepare(obj.sinceVersion())) {
            return false;
        }

        if ((generator().versionDependentCode()) && 
            (obj.sinceVersion() < ptr->sinceVersion())) {
            generator().logger().error("Currently version dependent members of variant are not supported!");
            return false;
        }

        m_members.push_back(std::move(ptr));
    }
    return true;
}

void VariantField::updateIncludesImpl(IncludesList& includes) const
{
    static const IncludesList List = {
        "comms/field/Variant.h",
        "<tuple>"
    };

    common::mergeIncludes(List, includes);

    for (auto& m : m_members) {
        m->updateIncludes(includes);
    }
}

void VariantField::updatePluginIncludesImpl(Field::IncludesList& includes) const
{
    for (auto& m : m_members) {
        m->updatePluginIncludes(includes);
    }
}

std::string VariantField::getClassDefinitionImpl(
    const std::string& scope,
    const std::string& className) const
{
    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("PREFIX", getClassPrefix(className)));
    replacements.insert(std::make_pair("CLASS_NAME", className));
    replacements.insert(std::make_pair("ORIG_CLASS_NAME", common::nameToClassCopy(name())));
    replacements.insert(std::make_pair("PROT_NAMESPACE", generator().mainNamespace()));
    replacements.insert(std::make_pair("FIELD_OPTS", getFieldOpts(scope)));
    replacements.insert(std::make_pair("NAME", getNameFunc()));
    replacements.insert(std::make_pair("READ", getRead()));
    replacements.insert(std::make_pair("WRITE", getCustomWrite()));
    replacements.insert(std::make_pair("LENGTH", getCustomLength()));
    replacements.insert(std::make_pair("VALID", getCustomValid()));
    replacements.insert(std::make_pair("REFRESH", getRefresh()));
    replacements.insert(std::make_pair("MEMBERS_STRUCT_DEF", getMembersDef(scope)));
    replacements.insert(std::make_pair("ACCESS", getAccess()));
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

std::string VariantField::getExtraDefaultOptionsImpl(const std::string& scope) const
{
    std::string memberScope = scope + common::nameToClassCopy(name()) + common::membersSuffixStr() + "::";
    StringsList options;
    options.reserve(m_members.size());
    for (auto& m : m_members) {
        auto opt = m->getDefaultOptions(memberScope);
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

std::string VariantField::getPluginAnonNamespaceImpl(
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

std::string VariantField::getPluginPropertiesImpl(bool serHiddenParam) const
{
    common::StringsList props;
    props.reserve(m_members.size() + 1);
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

    auto obj = variantFieldDslObj();
    if (obj.displayIdxReadOnlyHidden()) {
        props.push_back(".setIndexHidden()");
    }

    props.push_back(".serialisedHidden()");

    return common::listToString(props, "\n", common::emptyString());
}

std::string VariantField::getFieldOpts(const std::string& scope) const
{
    StringsList options;

    updateExtraOptions(scope, options);

    auto obj = variantFieldDslObj();
    auto idx = obj.defaultMemberIdx();
    if (idx < m_members.size()) {
        auto opt = "comms::option::DefaultVariantIndex<" + common::numToString(idx) + '>';
        common::addToList(opt, options);
    }

    // bool hasCustomReadRefresh =
    //     std::any_of(
    //         m_members.begin(), m_members.end(),
    //         [](auto& m) {
    //             assert(m);
    //             return m->hasCustomReadRefresh();
    //         });

    // if (hasCustomReadRefresh) {
    //     common::addToList("comms::option::HasCustomRead", options);
    //     common::addToList("comms::option::HasCustomRefresh", options);
    // }

    return common::listToString(options, ",\n", common::emptyString());
}

std::string VariantField::getMembersDef(const std::string& scope) const
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
        prefix += "template <typename TOpt = " + generator().mainNamespace() + "::" + common::defaultOptionsStr() + ">";
    }

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("CLASS_NAME", className));
    replacements.insert(std::make_pair("EXTRA_PREFIX", std::move(prefix)));
    replacements.insert(std::make_pair("MEMBERS_DEFS", common::listToString(membersDefs, "\n", common::emptyString())));
    replacements.insert(std::make_pair("MEMBERS", common::listToString(membersNames, ",\n", common::emptyString())));
    return common::processTemplate(MembersDefTemplate, replacements);

}

std::string VariantField::getAccess() const
{
    static const std::string Templ =
        "/// @brief Allow access to internal fields.\n"
        "/// @details See definition of @b COMMS_VARIANT_MEMBERS_ACCESS macro\n"
        "///     related to @b comms::field::Variant class from COMMS library\n"
        "///     for details.\n"
        "///\n"
        "///     The generated access functions are:\n"
        "#^#ACCESS_DOC#$#\n"
        "COMMS_VARIANT_MEMBERS_ACCESS(\n"
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
            "///     @li @b initField_" + namesList.back() +
            "() and @b accessField_" + namesList.back() + " - for " + scope +
            common::nameToClassCopy(m->name()) + " member field.";
        accessDocList.push_back(std::move(accessStr));
    }

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("ACCESS_DOC", common::listToString(accessDocList, "\n", common::emptyString())));
    replacements.insert(std::make_pair("NAMES", common::listToString(namesList, ",\n", common::emptyString())));
    return common::processTemplate(Templ, replacements);
}

std::string VariantField::getRead() const
{
    return getCustomRead();
    // auto customRead = getCustomRead();
    // if (!customRead.empty()) {
    //     return customRead;
    // }

    // return getReadForFields(m_members);
}

std::string VariantField::getRefresh() const
{
    return getCustomRefresh();
    // auto customRefresh = getCustomRefresh();
    // if (!customRefresh.empty()) {
    //     return customRefresh;
    // }

    // return getPublicRefreshForFields(m_members, false);
}

std::string VariantField::getPrivate() const
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

} // namespace commsdsl2comms