#pragma once

class SkipList
{
public:
    SkipList (jam::Document::Element* firstLine, unsigned int randomSeed)
        : levelSeed (randomSeed), levelGenerator (levelSeed)
    {
        jassert (firstLine != nullptr);
        jassert (firstLine->isTag (Id::line));

        auto& headEntry { nodes.add (std::make_unique<Node>()) };
        head = headEntry.get();

        tailChain.resize (maxLevel);

        for (auto level { 0 }; level < maxLevel; ++level)
            tailChain.set (level, head);

        build (firstLine);
    }

    ~SkipList() = default;

    void appendLine (jam::Document::Element* element)
    {
        jassert (element != nullptr);
        jassert (element->isTag (Id::line));

        for (auto level { 0 }; level < levelCount; ++level)
            tailChain.at (level)->width.set (level, tailChain.at (level)->width.at (level) + 1);

        const auto newLevel { getLevel() };

        if (newLevel > levelCount)
        {
            for (auto level { levelCount }; level < newLevel; ++level)
            {
                head->forward.add (nullptr);
                head->width.add (lineCount + 1);
            }

            levelCount = newLevel;
        }

        auto& nodeEntry { nodes.add (std::make_unique<Node>()) };
        auto* newNodePointer { nodeEntry.get() };
        newNodePointer->element = element;
        newNodePointer->forward.resize (newLevel);
        newNodePointer->width.resize (newLevel);

        for (auto level { 0 }; level < newLevel; ++level)
        {
            tailChain.at (level)->forward.set (level, newNodePointer);
            tailChain.set (level, newNodePointer);
        }

        ++lineCount;
    }

    jam::Document::Element* elementAt (int lineNumber) const
    {
        jassert (lineNumber >= 0);
        jassert (lineNumber < lineCount);

        const auto targetPosition { lineNumber + 1 };

        auto* current { head };
        auto currentPosition { 0 };

        for (auto level { levelCount - 1 }; level >= 0; --level)
        {
            while (current->forward.at (level) != nullptr
                and currentPosition + current->width.at (level) <= targetPosition)
            {
                currentPosition += current->width.at (level);
                current = current->forward.at (level);
            }
        }

        jassert (current != nullptr);

        return current->element;
    }

private:
    struct Node
    {
        jam::Document::Element* element { nullptr };
        jam::Array<Node*> forward;
        jam::Array<int> width;
    };

    int getLevel()
    {
        std::uniform_int_distribution<int> coinFlip { 0, 1 };
        auto level { 1 };

        while (level < maxLevel and coinFlip (levelGenerator) == 1)
            ++level;

        return level;
    }

    void build (jam::Document::Element* firstLine)
    {
        for (auto* element { firstLine }; element != nullptr; element = element->nextSibling)
            if (element->isTag (Id::line))
                appendLine (element);
    }

    jam::Owner<Node> nodes;
    Node* head { nullptr };
    jam::Array<Node*> tailChain;
    static constexpr int maxLevel { 32 };
    int levelCount { 0 };
    unsigned int levelSeed;
    std::mt19937 levelGenerator;
    int lineCount { 0 };
};
