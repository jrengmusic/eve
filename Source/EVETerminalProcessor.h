#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @class EVETerminalProcessor
 * @brief Terminal chain of the plugin: renders a markdown harness into a terminal row grid and carries the wire bytes to the text editor.
 *
 * `load()` prints a markdown file into the rows of the graphics engine. `process()` makes the engine write the
 * rows to the history FIFO and the live FIFO of the terminal processor. The text editor of the view
 * pulls the bytes through the getter in `userInterfaceGetters`.
 *
 * @par Thread contract
 * `setWinsize()` stores the size atomically, and `process()` reads it. `load()`, `prepare()` and `reset()` run on the message thread.
 */
class EVETerminalProcessor
{
public:
    /**
     * @brief Registers the user interface getters.
     * @param processorToUse  Audio processor that `load()` suspends while it prints. It must outlive this object.
     * @param modelToUse      Model that gives the history capacity and the appearance. It must outlive this object.
     */
    EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse);

    /**
     * @brief Sizes both FIFOs of the terminal processor.
     *
     * The live FIFO holds the bytes of one frame of the current window size. The history FIFO holds
     * the live bytes, the bytes of the rows that the engine holds now, and the bytes of a grid as wide as the
     * current window and as tall as the history capacity of the model (jassert > 0).
     *
     * @param newSpec  Processing spec; the processor keeps it for `load()`.
     */
    void prepare (const juce::dsp::ProcessSpec& newSpec);

    /** @brief Discards the bytes in both FIFOs. */
    void reset() noexcept;

    /** @brief Makes the engine write its ready rows to the FIFOs. The live area has the row count of the current window size. */
    void process() noexcept;

    /**
     * @brief Stores the window size that `prepare()` and `process()` read.
     * @param newWinsize  Size of the terminal view, in cells and in pixels.
     */
    void setWinsize (jam::TerminalWinsize newWinsize);

    /**
     * @brief Prints a markdown file into the rows of the graphics engine.
     *
     * The function maps the file, parses it as markdown and calls `addLines()` on the document to get its display lines.
     * It suspends the audio processor. It prepares the engine for a grid with the row count of the display lines
     * and the column count 0, and resets the rows of one frame. It then prints each display line into
     * the rows of the frame, from top to bottom. It calls `prepare()` with the stored spec and resumes the audio processor.
     *
     * @param harness  Markdown file to render (jassert that it maps).
     */
    void load (const juce::File& harness);

    jam::ViewManager::Getters userInterfaceGetters;     ///< Getters for the view. The constructor adds the text editor getter that pulls the bytes of both FIFOs.
    jam::ViewManager::Events chainEvents;               ///< Events of the chain. The class adds none.

private:
    /** @brief Adds the text editor getter keyed by the winsize type. It calls `jam::TerminalProcessor::pull()`. */
    void registerUserInterfaceGetters();

    /**
     * @brief Returns the number of engine rows that a display line takes.
     *
     * A line of a document block takes the rows of its gap height. Any other line takes one row
     * plus the top padding and the bottom padding of the line, in rows.
     *
     * @param line   The display line.
     * @param style  Style that gives the gap height and the padding.
     * @return The row count.
     */
    static int getNumRows (const jam::Document::Element& line, const jam::StyleMarkdown& style);

    /**
     * @brief Returns the number of engine rows that all display lines of a document take.
     * @param markdown  Document whose root holds the display lines.
     * @param style     Style that gives the gap heights and the paddings.
     * @return The sum of the row counts of the `Id::line` children of the root.
     */
    static int getNumRows (const jam::MarkdownDocument& markdown, const jam::StyleMarkdown& style);

    /**
     * @brief Prints a display line into its rows of the frame.
     *
     * The row after the top padding gets the cells of the line through `addRow()`. Every other row of the line gets its markers through `addMarkers()`.
     * A line of a document block prints nothing.
     *
     * @param writer  Write handle of the frame.
     * @param line    The display line.
     * @param row     First row of the line in the frame.
     * @param style   Style that gives the row count and the padding.
     * @return The first row after the line.
     */
    int addRows (const juce::AbstractFifo::ScopedWrite& writer, const jam::Document::Element& line, int row, const jam::StyleMarkdown& style);

    /**
     * @brief Prints the cells of a display line into one row of the frame.
     *
     * A line with an `Id::key` prints the key cells at column 0. The cells of the line follow at the left padding.
     * Each child line is a table column. It gets a column start marker at its column, and its cells follow.
     * The row ends with the markers of the line.
     *
     * @param writer  Write handle of the frame.
     * @param row     Row of the frame that receives the cells.
     * @param line    The display line.
     * @param style   Style that gives the padding.
     */
    void addRow (const juce::AbstractFifo::ScopedWrite& writer, int row, const jam::Document::Element& line, const jam::StyleMarkdown& style);

    /**
     * @brief Sets one atom of a target row from a cell of a display line.
     *
     * The atom gets a foreground colour id and a background colour id of the engine, and the default font.
     * The background is the pill colour when the cell belongs to a pill, and the cell background otherwise.
     * A grapheme atom takes its cluster from @p cells.
     *
     * @param targetRow     Row of the engine that receives the atom.
     * @param targetColumn  Column of the atom in @p targetRow.
     * @param cells         Cells of the display line.
     * @param index         Index of the cell in @p cells.
     * @param style         Style that gives the colours.
     */
    void setAtom (jam::AttributedString& targetRow, int targetColumn, const jam::AttributedString& cells, int index, const jam::StyleMarkdown& style);

    /**
     * @brief Prints cells into a row of the frame, from a column on.
     *
     * The function resizes the row to hold the cells without the object cells. An object cell adds an image
     * through `addImage()` at the current column and takes no column. Any other cell goes through `setAtom()`
     * and takes one column. The link runs of the cells go to the row, with their ranges moved to the columns.
     *
     * @param writer  Write handle of the frame.
     * @param row     Row of the frame.
     * @param column  First column to print into.
     * @param cells   Cells to print.
     * @param images  Image elements of the object cells, in the order of the cells.
     * @param style   Style that gives the colours and the image sizes.
     * @return The column after the last printed cell.
     */
    int addCells (const juce::AbstractFifo::ScopedWrite& writer, int row, int column, const jam::AttributedString& cells, const jam::Document::Elements& images, const jam::StyleMarkdown& style);

    /**
     * @brief Adds an inline image of an image element as a marker placement.
     *
     * The image is the diagram image of a mermaid element, or the raster image of the element otherwise (jassert not a vector image).
     * The marker bytes are the inline image sequence, sized in cells.
     *
     * @param writer        Write handle of the frame.
     * @param row           Row of the frame.
     * @param targetColumn  Column of the placement.
     * @param element       Image element.
     * @param style         Style that gives the cell size of the image.
     */
    void addImage (const juce::AbstractFifo::ScopedWrite& writer, int row, int targetColumn, const jam::Document::Element& element, const jam::StyleMarkdown& style);

    /**
     * @brief Returns the image of a mermaid diagram.
     *
     * The function parses the diagram text of the element and lays it out with the style.
     * It paints the diagram into an image that has the pixel size of the diagram cell bounds.
     *
     * @param element  Mermaid element whose first child holds the diagram text.
     * @param style    Style that gives the diagram style and the cell size.
     * @return The image of the diagram.
     */
    static juce::Image getDiagramImage (const jam::Document::Element& element, const jam::StyleMarkdown& style);

    /**
     * @brief Returns the number of object cells before a cell index.
     * @param cells  Cells of the display line.
     * @param end    Index after the last cell to count.
     * @return The number of object cells in the range from the first cell to @p end.
     */
    static int getNumObjects (const jam::AttributedString& cells, int end);

    /**
     * @brief Adds the line start marker and the fill marker of a line as one marker placement at column 0.
     *
     * The line start marker holds the indent of the line, in cells. A line that has a fill adds a fill marker
     * that spans the width of the line. The fill height is the height metric of the fill over the cell height of the line,
     * or 1 when the fill has no height metric. The function adds no fill when the colour maps to no colour id.
     *
     * @param writer  Write handle of the frame.
     * @param row     Row of the frame.
     * @param line    The display line.
     * @param style   Style that gives the indent, the fill colour and the fill height.
     */
    void addMarkers (const juce::AbstractFifo::ScopedWrite& writer, int row, const jam::Document::Element& line, const jam::StyleMarkdown& style);

    juce::AudioProcessor& processor;                    ///< Audio processor that `load()` suspends.
    jam::AudioModel& model;                             ///< Model that gives the history capacity and the appearance.
    juce::dsp::ProcessSpec spec { 44100.0, 1024, 2 };   ///< Spec of the last `prepare()`.
    std::atomic<int64_t> pendingWinsize { 0 };          ///< Packed `jam::TerminalWinsize` that `setWinsize()` stores.
    jam::TerminalProcessor terminalProcessor;           ///< History FIFO and live FIFO.

    static constexpr bool shouldUseInlineImages { true };   ///< Whether the engine presents image placements as inline images.

    jam::terminal::GraphicsEngine engine { shouldUseInlineImages };     ///< Row grid that `load()` prints into and `process()` presents.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVETerminalProcessor)
};
