#pragma once

static jam::Document::Element* getElementAt (const jam::Document::Element& root, int lineNumber)
{
    jassert (lineNumber >= 0);

    int currentLineNumber { 0 };

    for (auto* element : root)
    {
        if (element->isTag (Id::line))
        {
            if (currentLineNumber == lineNumber) return element;

            ++currentLineNumber;
        }
    }

    return nullptr;
}

static jam::Document::Element* getElementAt (const jam::AnsiDocument& document, int lineNumber)
{
    return getElementAt (*document.root, lineNumber);
}

class Anchor
{
public:
    Anchor (const jam::AnsiDocument& document, int startLineNumber)
        : currentElement (getElementAt (document, startLineNumber)),
          currentLineNumber (startLineNumber)
    {
        jassert (currentElement != nullptr);
    }

    void setLine (int newLineNumber)
    {
        jassert (newLineNumber >= 0);

        if (newLineNumber >= currentLineNumber)
        {
            auto* sibling { currentElement };

            while (currentLineNumber < newLineNumber)
            {
                sibling = sibling->nextSibling;
                jassert (sibling != nullptr);

                if (sibling->isTag (Id::line))
                    ++currentLineNumber;
            }

            currentElement = sibling;
        }
        else
        {
            currentElement = getElementAt (*currentElement->parent, newLineNumber);
            currentLineNumber = newLineNumber;
        }
    }

    jam::Document::Element* elementAt (int lineNumber)
    {
        setLine (lineNumber);
        return get();
    }

    jam::Document::Element* get() const noexcept { return currentElement; }

private:
    jam::Document::Element* currentElement;
    int currentLineNumber;
};
