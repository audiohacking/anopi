#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

inline void paintAnopiPad (juce::Graphics& g, juce::Rectangle<float> cell,
                           bool on, const juce::String& text, float fontSize)
{
    auto r = cell.reduced (1.0f);
    const float rad = 4.0f;
    g.setColour (on ? juce::Colour (0xffeef4b4) : juce::Colour (0xffd2dcc8));
    g.fillRoundedRectangle (r, rad);
    g.setColour (on ? juce::Colour (0xff2a3328) : juce::Colour (0xff7e8b74));
    g.drawRoundedRectangle (r, rad, on ? 1.7f : 1.0f);
    g.setColour (juce::Colour (0xff243028));
    g.setFont (juce::FontOptions (fontSize).withStyle ("Bold"));
    g.drawText (text, r, juce::Justification::centred, false);
}

struct PadItem
{
    juce::String name;
    int value = 0;
};

class PadBank : public juce::Component
{
public:
    PadBank (juce::String titleToUse, std::vector<juce::String> namesToUse, int columnsToUse)
        : title (std::move (titleToUse)), columns (std::max (1, columnsToUse))
    {
        items.reserve (namesToUse.size());
        for (int i = 0; i < (int) namesToUse.size(); ++i)
            items.push_back ({ namesToUse[(size_t) i], i });
        setInterceptsMouseClicks (true, false);
    }

    PadBank (juce::String titleToUse, std::vector<PadItem> itemsToUse, int columnsToUse)
        : title (std::move (titleToUse)), items (std::move (itemsToUse)), columns (std::max (1, columnsToUse))
    {
        setInterceptsMouseClicks (true, false);
    }

    std::function<void (int value)> onSelect;

    void setSelected (int value)
    {
        if (value == selectedValue)
            return;
        selectedValue = value;
        repaint();
    }

    int getSelected() const noexcept { return selectedValue; }

    void nudge (int dx, int dy)
    {
        const int n = (int) items.size();
        if (n <= 0)
            return;

        const int rows = (n + columns - 1) / columns;
        int vis = visualIndexOf (selectedValue);
        int col = vis % columns;
        int row = vis / columns;
        col = (col + dx + columns) % columns;
        row = (row + dy + rows) % rows;
        int next = row * columns + col;
        if (next >= n)
            next = n - 1;
        selectVisual (next);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xffe7eee0));
        g.fillRoundedRectangle (r, 6.0f);

        auto header = r.removeFromTop (16.0f);
        g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
        g.setColour (juce::Colour (0xff5a6658));
        g.drawText (title, header.reduced (8.0f, 0.0f), juce::Justification::centredLeft);

        const int n = (int) items.size();
        if (n == 0)
            return;

        const int rows = (n + columns - 1) / columns;
        const float gap = 3.0f;
        const float w = (r.getWidth() - gap * (float) (columns + 1)) / (float) columns;
        const float h = (r.getHeight() - gap * (float) (rows + 1)) / (float) rows;
        const float font = juce::jlimit (9.0f, 12.5f, h * 0.42f);

        for (int i = 0; i < n; ++i)
        {
            const bool on = items[(size_t) i].value == selectedValue;
            paintAnopiPad (g, cellAt (r, i, w, h, gap), on, items[(size_t) i].name, font);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto r = getLocalBounds().toFloat();
        r.removeFromTop (16.0f);
        const int n = (int) items.size();
        const int rows = (n + columns - 1) / columns;
        const float gap = 3.0f;
        const float w = (r.getWidth() - gap * (float) (columns + 1)) / (float) columns;
        const float h = (r.getHeight() - gap * (float) (rows + 1)) / (float) rows;

        for (int i = 0; i < n; ++i)
        {
            if (cellAt (r, i, w, h, gap).contains (e.position))
            {
                selectVisual (i);
                return;
            }
        }
    }

private:
    juce::Rectangle<float> cellAt (juce::Rectangle<float> r, int i, float w, float h, float gap) const
    {
        const int row = i / columns;
        const int col = i % columns;
        return { r.getX() + gap + (w + gap) * (float) col,
                 r.getY() + gap + (h + gap) * (float) row,
                 w, h };
    }

    int visualIndexOf (int value) const
    {
        for (int i = 0; i < (int) items.size(); ++i)
            if (items[(size_t) i].value == value)
                return i;
        return 0;
    }

    void selectVisual (int visual)
    {
        visual = juce::jlimit (0, (int) items.size() - 1, visual);
        selectedValue = items[(size_t) visual].value;
        repaint();
        if (onSelect)
            onSelect (selectedValue);
    }

    juce::String title;
    std::vector<PadItem> items;
    int columns = 4;
    int selectedValue = 0;
};
