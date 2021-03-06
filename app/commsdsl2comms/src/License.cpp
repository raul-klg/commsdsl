//
// Copyright 2019 - 2020 (C). Alex Robenko. All rights reserved.
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

#include "License.h"

#include <fstream>
#include <vector>
#include <string>

#include <boost/filesystem.hpp>

#include "Generator.h"
#include "common.h"

namespace bf = boost::filesystem;

namespace commsdsl2comms
{

bool License::write(Generator& generator)
{
    License obj(generator);
    return
        obj.writeTxt();
}

bool License::writeTxt() const
{
    static const std::string LicenseFile("LICENSE.txt");
    auto filePath = m_generator.startLicenseWrite(LicenseFile);

    if (filePath.empty()) {
        return true;
    }

    std::ofstream stream(filePath);
    if (!stream) {
        m_generator.logger().error("Failed to open \"" + filePath + "\" for writing.");
        return false;
    }

    static const std::string Template = 
        "This code has been generated by the commsdsl2comms[1] application and has no license,\n"
        "the vendor is free to pick one. HOWEVER, the generated code uses COMMS Library[2],\n"
        "which is provided under GPLv3 / Commercial dual licensing scheme. Unless commercial closed\n"
        "source license is obtained for the  COMMS library[2], the generated code as well as integrating\n"
        "business logic must remain open source and the picked license be compatible with GPLv3.\n\n"
        "[1]: https://github.com/arobenko/commsdsl\n"
        "[2]: https://github.com/arobenko/comms_champion#comms-library\n\n"
        "#^#APPEND#$#\n"
        "\n";

    common::ReplacementMap replacements;
    replacements.insert(std::make_pair("APPEND", m_generator.getExtraAppendForFile(LicenseFile)));

    stream << common::processTemplate(Template, replacements);

    stream.flush();
    if (!stream.good()) {
        m_generator.logger().error("Failed to write \"" + filePath + "\".");
        return false;
    }
    return true;
}

} // namespace commsdsl2comms
