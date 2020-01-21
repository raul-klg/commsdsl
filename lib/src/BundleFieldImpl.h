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

#pragma once

#include "commsdsl/BundleField.h"
#include "FieldImpl.h"
#include "AliasImpl.h"

namespace commsdsl
{

class BundleFieldImpl : public FieldImpl
{
    using Base = FieldImpl;
public:
    BundleFieldImpl(::xmlNodePtr node, ProtocolImpl& protocol);
    BundleFieldImpl(const BundleFieldImpl& other);
    using Members = BundleField::Members;
    using AliasesList = BundleField::Aliases;


    const FieldsList& members() const
    {
        return m_members;
    }

    Members membersList() const;
    AliasesList aliasesList() const;

protected:

    virtual Kind kindImpl() const override final;
    virtual Ptr cloneImpl() const override final;
    virtual const XmlWrap::NamesList& extraPropsNamesImpl() const override final;
    virtual const XmlWrap::NamesList& extraChildrenNamesImpl() const override final;
    virtual bool reuseImpl(const FieldImpl &other) override final;
    virtual bool parseImpl() override final;
    virtual std::size_t minLengthImpl() const override final;
    virtual std::size_t maxLengthImpl() const override final;
    virtual bool strToNumericImpl(const std::string& ref, std::intmax_t& val, bool& isBigUnsigned) const override final;
    virtual bool strToFpImpl(const std::string& ref, double& val) const override final;
    virtual bool strToBoolImpl(const std::string& ref, bool& val) const override final;
    virtual bool strToStringImpl(const std::string& ref, std::string& val) const override final;
    virtual bool strToDataImpl(const std::string& ref, std::vector<std::uint8_t>& val) const override final;
    virtual bool verifyAliasedMemberImpl(const std::string& fieldName) const override final;

private:
    bool updateMembers();
    bool updateAliases();

    FieldsList m_members;
    std::vector<AliasImplPtr> m_aliases;
};

} // namespace commsdsl
