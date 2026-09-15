#pragma once

template <int arity>
class CountedTree
{
public:
    explicit CountedTree (jam::Document::Element* firstLine)
    {
        jassert (firstLine != nullptr);
        jassert (firstLine->isTag (Id::line));

        jam::Document::Elements lineElements;

        for (auto* element { firstLine }; element != nullptr; element = element->nextSibling)
            if (element->isTag (Id::line))
                lineElements.add (element);

        root = build (lineElements);
        lineCount = lineElements.size();
    }

    void appendLine (jam::Document::Element* element)
    {
        jassert (element != nullptr);
        jassert (element->isTag (Id::line));

        if (root == nullptr)
        {
            auto leaf { std::make_unique<Node>() };
            leaf->isLeaf = true;
            leaf->leafEntries.add (element);
            root = std::move (leaf);
        }
        else
        {
            auto newSibling { appendToNode (*root, element) };

            if (newSibling != nullptr)
            {
                auto newRoot { std::make_unique<Node>() };
                newRoot->isLeaf = false;
                newRoot->childCounts.add (getLineCount (*root));
                newRoot->childCounts.add (getLineCount (*newSibling));
                newRoot->children.add (std::move (root));
                newRoot->children.add (std::move (newSibling));
                root = std::move (newRoot);
            }
        }

        ++lineCount;
    }

    jam::Document::Element* elementAt (int lineNumber) const
    {
        jassert (lineNumber >= 0);
        jassert (lineNumber < lineCount);

        auto* current { root.get() };
        auto remaining { lineNumber };

        while (not current->isLeaf)
        {
            auto childIndex { 0 };

            while (remaining >= current->childCounts.at (childIndex))
            {
                remaining -= current->childCounts.at (childIndex);
                ++childIndex;
            }

            current = current->children.at (childIndex).get();
        }

        return current->leafEntries.at (remaining);
    }

private:
    struct Node
    {
        bool isLeaf { true };
        jam::Document::Elements leafEntries;
        jam::Array<std::unique_ptr<Node>> children;
        jam::Array<int> childCounts;
    };

    static int getLineCount (const Node& node)
    {
        auto nodeLineCount { 0 };

        if (node.isLeaf)
        {
            nodeLineCount = node.leafEntries.size();
        }
        else
        {
            for (const auto childCount : node.childCounts)
                nodeLineCount += childCount;
        }

        return nodeLineCount;
    }

    static jam::Array<std::unique_ptr<Node>> buildLeaves (const jam::Document::Elements& lineElements)
    {
        jam::Array<std::unique_ptr<Node>> leaves;

        for (auto groupStart { 0 }; groupStart < lineElements.size(); groupStart += arity)
        {
            auto leaf { std::make_unique<Node>() };
            leaf->isLeaf = true;

            const auto groupEnd { juce::jmin (groupStart + arity, lineElements.size()) };

            for (auto elementIndex { groupStart }; elementIndex < groupEnd; ++elementIndex)
                leaf->leafEntries.add (lineElements.at (elementIndex));

            leaves.add (std::move (leaf));
        }

        return leaves;
    }

    static jam::Array<std::unique_ptr<Node>> buildParentLevel (jam::Array<std::unique_ptr<Node>>& currentLevel)
    {
        jam::Array<std::unique_ptr<Node>> parentLevel;

        for (auto groupStart { 0 }; groupStart < currentLevel.size(); groupStart += arity)
        {
            auto parent { std::make_unique<Node>() };
            parent->isLeaf = false;

            const auto groupEnd { juce::jmin (groupStart + arity, currentLevel.size()) };

            for (auto childIndex { groupStart }; childIndex < groupEnd; ++childIndex)
            {
                parent->childCounts.add (getLineCount (*currentLevel.at (childIndex)));
                parent->children.add (std::move (currentLevel.at (childIndex)));
            }

            parentLevel.add (std::move (parent));
        }

        return parentLevel;
    }

    static std::unique_ptr<Node> build (const jam::Document::Elements& lineElements)
    {
        auto currentLevel { buildLeaves (lineElements) };

        while (currentLevel.size() > 1)
            currentLevel = buildParentLevel (currentLevel);

        if (not currentLevel.isEmpty())
            return std::move (currentLevel.at (0));

        return nullptr;
    }

    static std::unique_ptr<Node> appendToNode (Node& node, jam::Document::Element* element)
    {
        std::unique_ptr<Node> newSibling;

        if (node.isLeaf)
        {
            node.leafEntries.add (element);

            if (node.leafEntries.size() > arity)
                newSibling = splitLeaf (node);
        }
        else
        {
            const auto lastChildIndex { node.children.size() - 1 };
            auto newChildSibling { appendToNode (*node.children.at (lastChildIndex), element) };

            node.childCounts.at (lastChildIndex) = getLineCount (*node.children.at (lastChildIndex));

            if (newChildSibling != nullptr)
            {
                node.childCounts.add (getLineCount (*newChildSibling));
                node.children.add (std::move (newChildSibling));

                if (node.children.size() > arity)
                    newSibling = splitInternal (node);
            }
        }

        return newSibling;
    }

    static std::unique_ptr<Node> splitLeaf (Node& node)
    {
        auto newLeaf { std::make_unique<Node>() };
        newLeaf->isLeaf = true;

        const auto overflowIndex { node.leafEntries.size() - 1 };
        newLeaf->leafEntries.add (node.leafEntries.at (overflowIndex));
        node.leafEntries.remove (overflowIndex);

        return newLeaf;
    }

    static std::unique_ptr<Node> splitInternal (Node& node)
    {
        auto newInternal { std::make_unique<Node>() };
        newInternal->isLeaf = false;

        const auto overflowIndex { node.children.size() - 1 };
        newInternal->childCounts.add (node.childCounts.at (overflowIndex));
        newInternal->children.add (std::move (node.children.at (overflowIndex)));

        node.children.remove (overflowIndex);
        node.childCounts.remove (overflowIndex);

        return newInternal;
    }

    std::unique_ptr<Node> root;
    int lineCount { 0 };
};
