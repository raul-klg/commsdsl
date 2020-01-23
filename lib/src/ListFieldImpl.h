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

#include <cstdint>
#include <utility>
#include <vector>

#include "commsdsl/Endian.h"
#include "FieldImpl.h"

namespace commsdsl
{

class ListFieldImpl : public FieldImpl
{
    using Base = FieldImpl;
public:

    ListFieldImpl(::xmlNodePtr node, ProtocolImpl& protocol);
    ListFieldImpl(const ListFieldImpl& other);

    std::size_t count() const
    {
        return m_state.m_count;
    }

    bool hasElementField() const
    {
        return (m_state.m_extElementField != nullptr) ||
               static_cast<bool>(m_elementField);
    }

    Field elementField() const
    {
        if (m_state.m_extElementField != nullptr) {
            return Field(m_state.m_extElementField);
        }

        return Field(m_elementField.get());
    }


    bool hasCountPrefixField() const
    {
        return (m_state.m_extCountPrefixField != nullptr) ||
               static_cast<bool>(m_countPrefixField);
    }

    Field countPrefixField() const
    {
        if (m_state.m_extCountPrefixField != nullptr) {
            return Field(m_state.m_extCountPrefixField);
        }

        return Field(m_countPrefixField.get());
    }

    const std::string& detachedCountPrefixFieldName() const
    {
        return m_state.m_detachedCountPrefixField;
    }

    bool hasLengthPrefixField() const
    {
        return (m_state.m_extLengthPrefixField != nullptr) ||
               static_cast<bool>(m_lengthPrefixField);
    }

    Field lengthPrefixField() const
    {
        if (m_state.m_extLengthPrefixField != nullptr) {
            return Field(m_state.m_extLengthPrefixField);
        }

        return Field(m_lengthPrefixField.get());
    }

    const std::string& detachedLengthPrefixFieldName() const
    {
        return m_state.m_detachedLengthPrefixField;
    }

    bool hasElemLengthPrefixField() const
    {
        return (m_state.m_extElemLengthPrefixField != nullptr) ||
               static_cast<bool>(m_elemLengthPrefixField);
    }

    Field elemLengthPrefixField() const
    {
        if (m_state.m_extElemLengthPrefixField != nullptr) {
            return Field(m_state.m_extElemLengthPrefixField);
        }

        return Field(m_elemLengthPrefixField.get());
    }

    const std::string& detachedElemLengthPrefixFieldName() const
    {
        return m_state.m_detachedElemLengthPrefixField;
    }

    bool elemFixedLength() const
    {
        return m_state.m_elemFixedLength;
    }


protected:
    virtual Kind kindImpl() const override final;
    virtual Ptr cloneImpl() const override final;
    virtual const XmlWrap::NamesList& extraPropsNamesImpl() const override final;
    virtual const XmlWrap::NamesList& extraPossiblePropsNamesImpl() const override final;
    virtual const XmlWrap::NamesList& extraChildrenNamesImpl() const override final;
    virtual bool reuseImpl(const FieldImpl& other) override final;
    virtual bool parseImpl() override final;
    virtual bool verifySiblingsImpl(const FieldsList& fields) const override final;
    virtual std::size_t minLengthImpl() const override final;
    virtual std::size_t maxLengthImpl() const override final;

private:
    void cloneFields(const ListFieldImpl& other);
    bool updateElement();
    bool updateCount();
    bool updateCountPrefix();
    bool updateLengthPrefix();
    bool updateElemLengthPrefix();
    bool updateElemFixedLength();
    bool checkElementFromRef();
    bool checkElementAsChild();
    bool checkPrefixFromRef(
        const std::string& type,
        const FieldImpl*& extField,
        FieldImplPtr& locField,
        std::string& detachedPrefix);
    bool checkPrefixAsChild(
        const std::string& type,
        const FieldImpl*& extField,
        FieldImplPtr& locField,
        std::string& detachedPrefix);
    const FieldImpl* getCountPrefixField() const;
    const FieldImpl* getLengthPrefixField() const;
    bool verifySiblingsForPrefix(const FieldsList& fields, const std::string& detachedName) const;

    struct State
    {
        std::size_t m_count = 0U;
        const FieldImpl* m_extElementField = nullptr;
        const FieldImpl* m_extCountPrefixField = nullptr;
        const FieldImpl* m_extLengthPrefixField = nullptr;
        const FieldImpl* m_extElemLengthPrefixField = nullptr;
        std::string m_detachedCountPrefixField;
        std::string m_detachedLengthPrefixField;
        std::string m_detachedElemLengthPrefixField;
        bool m_elemFixedLength = false;
    };

    State m_state;
    FieldImplPtr m_elementField;
    FieldImplPtr m_countPrefixField;
    FieldImplPtr m_lengthPrefixField;
    FieldImplPtr m_elemLengthPrefixField;
};

} // namespace commsdsl
