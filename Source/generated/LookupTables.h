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

/**
 * @file LookupTables.h
 * @brief Product lookup tables.
 */

#pragma once

namespace map
{
/*_____________________________________________________________________________*/

/** @brief Developer credit names displayed by the about-box scrambled-text animation. */
inline constexpr jam::LookupTable<int, const char*, 22> credits {
    {
        { 0, "Arie Ardiansyah" },
        { 1, "Bramantyo Ibrahim" },
        { 2, "Adhitya Wibisana" },
        { 3, "Iman Amarullah" },
        { 4, "Pratama Kusuma" },
        { 5, "Grahadea Kusuf" },
        { 6, "Juan Prahamma" },
        { 7, "Intan Zariska" },
        { 8, "Dimitar Nalbantov" },
        { 9, "Rathomi Trinugraha" },
        { 10, "Sundawan Sukmaya" },
        { 11, "Bayu Ardianto" },
        { 12, "Muhammad Yusreza" },
        { 13, "Abshar Plastizsa" },
        { 14, "Yudha Adie Putra" },
        { 15, "Rizki Firmansyah" },
        { 16, "Efril Nidyaprayoga" },
        { 17, "Syifa Nurul Asyfia" },
        { 18, "Fachrur Riaz Habullah" },
        { 19, "M. Rafly Indrakusumah" },
        { 20, "Tama Reza Setiawan" },
        { 21, "Rifqi Dewataprana" },
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace map
