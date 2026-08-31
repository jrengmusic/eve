/*******************************************************************************
                        Codegen Annotated Source of Truth
————————————————————————————————————————————————————————————————————————————————

            ░░████████████░░████████████░░████████████░░████████████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████        ░░████  ░░████░░████            ░░████
            ░░████        ░░████████████░░████████████    ░░████
            ░░████        ░░████  ░░████        ░░████    ░░████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████████████░░████  ░░████░░████████████    ░░████

————————————————————————————————————————————————————————————————————————————————
                         FOR YOUR EYES ONLY, DO NOT EDIT
********************************************************************************/

#pragma once

namespace ProjectInfo
{
/*_____________________________________________________________________________*/

/**
 * @brief Project metadata — the ProjectInfo namespace, generated.
 *
 * Every field is a complete literal; nothing downstream derives, concatenates, or restates a value.
 */

    static constexpr const char* const projectName      { "EVE"                    };///< Product name.
    static constexpr const char* const companyName      { "JRENG"                  };///< Company name.
    static constexpr const char* const legalCompanyName { "PT JRENG Teknika"       };///< Full legal company name.
    static constexpr const char* const versionString    { "0.1.0"                  };///< Product version string.
    static constexpr int               versionNumber    { 0x100                    };///< Product version, JUCE hex encoding.
    static constexpr const char* const productWebsite   { "https://jrengmusic.com" };///< Product website URL.
    static constexpr const char* const presetExtension  { "endp"                   };///< Preset file extension, without the leading dot.
    static constexpr const char* const presetDefault    { "INIT"                   };///< Default init preset name, without the extension.

/**______________________________END OF NAMESPACE______________________________*/
}// namespace ProjectInfo
