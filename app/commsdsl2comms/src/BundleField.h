#pragma once

#include "commsdsl/BundleField.h"

#include "Field.h"
#include "common.h"

namespace commsdsl2comms
{

class BundleField : public Field
{
    using Base = Field;
public:
    BundleField(Generator& generator, commsdsl::Field field) : Base(generator, field) {}

protected:
    virtual bool prepareImpl() override;
    virtual void updateIncludesImpl(IncludesList& includes) const override;
    virtual void updatePluginIncludesImpl(IncludesList& includes) const override;
    virtual std::size_t minLengthImpl() const override;
    virtual std::string getClassDefinitionImpl(
        const std::string& scope,
        const std::string& className) const override;
    virtual std::string getExtraDefaultOptionsImpl(const std::string& scope) const override;
    virtual std::string getPluginAnonNamespaceImpl(
        const std::string& scope,
        bool forcedSerialisedHidden,
        bool serHiddenParam) const override;
    virtual std::string getPluginPropertiesImpl(bool serHiddenParam) const override;

private:
    using StringsList = common::StringsList;

    std::string getFieldOpts(const std::string& scope) const;
    std::string getMembersDef(const std::string& scope, const std::string& className) const;
    std::string getAccess(const std::string& className) const;
    std::string getRead() const;
    std::string getRefresh() const;
    std::string getPrivate() const;

    commsdsl::BundleField bundleFieldDslObj() const
    {
        return commsdsl::BundleField(dslObj());
    }

    std::vector<FieldPtr> m_members;
};

inline
FieldPtr createBundleField(Generator& generator, commsdsl::Field field)
{
    return std::make_unique<BundleField>(generator, field);
}

} // namespace commsdsl2comms
