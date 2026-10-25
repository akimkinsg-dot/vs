#pragma once
#include "PluginProcessor.h"

struct DragBox : juce::Component {
  explicit DragBox(VibeSparkProcessor& p) : proc(p) { setMouseCursor(juce::MouseCursor::DraggingHandCursor); }
  void paint(juce::Graphics& g) override {
    g.setColour(juce::Colour(0xff40267a)); g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1), 10.0f);
    g.setColour(juce::Colours::white); g.setFont(16.0f);
    g.drawText("DRAG MIDI  >>  drop on DAW track", getLocalBounds(), juce::Justification::centred);
  }
  void mouseDrag(const juce::MouseEvent& e) override {
    if (!dragging && e.getDistanceFromDragStart() > 4) {
      dragging = true;
      auto f = proc.writeMidiFile();
      if (f.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles(juce::StringArray(f.getFullPathName()), false, this, [this] { dragging = false; });
      else dragging = false;
    }
  }
  void mouseUp(const juce::MouseEvent&) override { dragging = false; }
  VibeSparkProcessor& proc; bool dragging = false;
};

struct MoodPad : juce::Component, private juce::Timer {
  enum Mode { MapMode, AnimMode, InnerMode };
  static constexpr float VX = 14.0f, VY = 14.0f, VW = 652.0f, VH = 412.0f, INFO_H = 58.0f;

  static int heightFor(int w) { return (int) std::lround((float) w * VH / VW + INFO_H); }

  explicit MoodPad(VibeSparkProcessor& p) : proc(p) {
    back.setButtonText("< Back to map");
    back.onClick = [this] { leave(); };
    addChildComponent(back);
    startTimerHz(60);
  }

  void resized() override { back.setBounds(getWidth() - 116, getHeight() - (int) INFO_H + 4, 116, 24); }

  juce::AffineTransform viewT() const {
    auto r = getLocalBounds().toFloat(); r.removeFromBottom(INFO_H);
    const float k = std::min(r.getWidth() / VW, r.getHeight() / VH);
    return juce::AffineTransform::translation(-VX, -VY).scaled(k)
        .translated(r.getX() + (r.getWidth() - VW * k) * 0.5f, r.getY() + (r.getHeight() - VH * k) * 0.5f);
  }
  juce::Point<float> toView(juce::Point<float> p) const { return p.transformedBy(viewT().inverted()); }

  static float sm(float t) { t = juce::jlimit(0.0f, 1.0f, t); return t * t * (3.0f - 2.0f * t); }
  static void setF(juce::Graphics& g, float h, bool bold) { g.setFont(h); if (bold) g.setFont(g.getCurrentFont().boldened()); }
  static juce::String U(const char* s) { return juce::String::fromUTF8(s); }
  static juce::String dot() { return U(" Â· "); }

  void enter(int i) {
    if (mode != MapMode) return;
    active = i; mode = AnimMode; animFrom = 0.0f; animTo = 1.0f; animStart = juce::Time::getMillisecondCounterHiRes();
    cur = tg = { 340.0f, 225.0f }; sent = cur;
    proc.setMood(i, cur.x, cur.y, true);
    computeWeights();
    setMouseCursor(juce::MouseCursor::NormalCursor);
  }
  void leave() {
    if (mode != InnerMode) return;
    mode = AnimMode; animFrom = 1.0f; animTo = 0.0f; animStart = juce::Time::getMillisecondCounterHiRes();
    back.setVisible(false);
  }

  void timerCallback() override {
    if (mode == AnimMode) {
      const float k = juce::jlimit(0.0f, 1.0f, (float) ((juce::Time::getMillisecondCounterHiRes() - animStart) / 700.0));
      t = animFrom + (animTo - animFrom) * sm(k);
      if (k >= 1.0f) {
        t = animTo;
        if (animTo > 0.5f) { mode = InnerMode; back.setVisible(true); }
        else { mode = MapMode; active = -1; }
      }
      repaint();
    } else if (mode == InnerMode) {
      const float dx = tg.x - cur.x, dy = tg.y - cur.y;
      if (std::abs(dx) + std::abs(dy) > 0.05f) {
        cur.x += dx * 0.22f; cur.y += dy * 0.22f;
        computeWeights();
        if (std::hypot(cur.x - sent.x, cur.y - sent.y) > 1.5f) { sent = cur; proc.setMood(active, cur.x, cur.y, false); }
        repaint();
      }
    }
  }

  int cornerAt(juce::Point<float> v) const {
    for (int i = 0; i < 4; ++i)
      if (std::hypot(v.x - CORNERS[i].cx, v.y - CORNERS[i].cy) <= 54.0f) return i;
    return -1;
  }
  void setTarget(juce::Point<float> v) { tg = { juce::jlimit(32.0f, 648.0f, v.x), juce::jlimit(32.0f, 408.0f, v.y) }; }
  void mouseMove(const juce::MouseEvent& e) override {
    setMouseCursor(mode == MapMode && cornerAt(toView(e.position)) >= 0 ? juce::MouseCursor::PointingHandCursor
                                                                         : juce::MouseCursor::NormalCursor);
  }
  void mouseDown(const juce::MouseEvent& e) override {
    const auto v = toView(e.position);
    if (mode == MapMode) { const int i = cornerAt(v); if (i >= 0) enter(i); }
    else if (mode == InnerMode) setTarget(v);
  }
  void mouseDrag(const juce::MouseEvent& e) override { if (mode == InnerMode) setTarget(toView(e.position)); }

  void computeWeights() {
    if (active < 0) return;
    moodWeights(active, cur.x, cur.y, w);
    nearest = 0; second = -1;
    for (int j = 1; j < 8; ++j) if (w[j] > w[nearest]) nearest = j;
    for (int j = 0; j < 8; ++j) if (j != nearest && (second < 0 || w[j] > w[second])) second = j;
  }

  void paint(juce::Graphics& g) override {
    {
      juce::Graphics::ScopedSaveState sss(g);
      g.addTransform(viewT());
      juce::Path frame; frame.addRoundedRectangle(20.0f, 20.0f, 640.0f, 400.0f, 12.0f);
      g.setColour(juce::Colour(0xff1a1530)); g.fillPath(frame);
      g.reduceClipRegion(frame);

      if (mode == MapMode) drawMap(g, 1.0f);
      else if (mode == InnerMode) drawInner(g, 1.0f);
      else {
        const Corner& c = CORNERS[active];
        const float mapA = 1.0f - sm(t / 0.7f), inA = sm((t - 0.3f) / 0.7f);
        if (mapA > 0.001f) {
          juce::Graphics::ScopedSaveState s2(g);
          const float s = 1.0f + 5.0f * t;
          g.addTransform(juce::AffineTransform::scale(s, s, c.cx, c.cy));
          drawMap(g, mapA);
        }
        if (inA > 0.001f) {
          juce::Graphics::ScopedSaveState s3(g);
          const float s = 0.2f + 0.8f * t;
          g.addTransform(juce::AffineTransform::scale(s, s, c.cx, c.cy));
          drawInner(g, inA);
        }
      }
    }
    {
      juce::Graphics::ScopedSaveState sss(g);
      g.addTransform(viewT());
      g.setColour(juce::Colours::white.withAlpha(0.14f));
      g.drawRoundedRectangle(20.0f, 20.0f, 640.0f, 400.0f, 12.0f, 1.0f);
    }
    paintInfo(g);
  }

  void drawMap(juce::Graphics& g, float alpha) {
    juce::Graphics::ScopedSaveState sss(g);
    if (alpha < 0.999f) g.beginTransparencyLayer(alpha);

    setF(g, 14.5f, false); g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawText("warm", juce::Rectangle<float>(34.0f, 215.0f, 120.0f, 18.0f), juce::Justification::left);
    g.drawText("cold", juce::Rectangle<float>(526.0f, 215.0f, 120.0f, 18.0f), juce::Justification::right);
    g.drawText("tap a circle", juce::Rectangle<float>(240.0f, 215.0f, 200.0f, 18.0f), juce::Justification::centred);

    for (int i = 0; i < 4; ++i) {
      const Corner& c = CORNERS[i]; const juce::Colour col(c.col);
      g.setColour(col.withAlpha(0.20f)); g.fillEllipse(c.cx - 54.0f, c.cy - 54.0f, 108.0f, 108.0f);
      g.setColour(col.withAlpha(0.95f)); g.drawEllipse(c.cx - 54.0f, c.cy - 54.0f, 108.0f, 108.0f, 1.3f);
      for (int j = 0; j < 8; ++j) {
        const Shade& s = SHADES[i * 8 + j];
        const float px = c.cx + (s.x - 340.0f) / 270.0f * 34.0f, py = c.cy + (s.y - 225.0f) / 135.0f * 30.0f;
        g.setColour(col.withAlpha(0.8f)); g.fillEllipse(px - 2.6f, py - 2.6f, 5.2f, 5.2f);
      }
      const bool top = c.cy < 220.0f;
      const float ty = top ? c.cy - 54.0f - 26.0f : c.cy + 54.0f + 22.0f;
      setF(g, 16.5f, true); g.setColour(juce::Colours::white.withAlpha(0.95f));
      g.drawText(juce::String(c.name) + dot() + c.tag, juce::Rectangle<float>(c.cx - 160.0f, ty - 14.0f, 320.0f, 20.0f), juce::Justification::centred);
      setF(g, 14.0f, false); g.setColour(juce::Colours::white.withAlpha(0.6f));
      g.drawText(c.desc, juce::Rectangle<float>(c.cx - 160.0f, ty + 3.0f, 320.0f, 18.0f), juce::Justification::centred);
    }
    if (alpha < 0.999f) g.endTransparencyLayer();
  }

  void drawInner(juce::Graphics& g, float alpha) {
    if (active < 0) return;
    juce::Graphics::ScopedSaveState sss(g);
    if (alpha < 0.999f) g.beginTransparencyLayer(alpha);
    const Corner& c = CORNERS[active]; const juce::Colour col(c.col);

    g.setColour(juce::Colour(0xff1a1530)); g.fillRoundedRectangle(20.0f, 20.0f, 640.0f, 400.0f, 12.0f);
    g.setColour(col.withAlpha(0.9f)); g.drawRoundedRectangle(20.0f, 20.0f, 640.0f, 400.0f, 12.0f, 1.3f);

    setF(g, 17.0f, true); g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.drawText(juce::String(c.name) + dot() + c.tag, juce::Rectangle<float>(36.0f, 32.0f, 420.0f, 20.0f), juce::Justification::left);
    setF(g, 14.0f, false); g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawText(c.desc, juce::Rectangle<float>(36.0f, 53.0f, 420.0f, 18.0f), juce::Justification::left);

    const juce::String aL = U("â"), aR = U("â"), aU = U("â"), aD = U("â");
    g.drawText(aL + " " + c.axL, juce::Rectangle<float>(34.0f, 398.0f, 260.0f, 18.0f), juce::Justification::left);
    g.drawText(juce::String(c.axR) + " " + aR, juce::Rectangle<float>(386.0f, 398.0f, 260.0f, 18.0f), juce::Justification::right);
    g.drawText(juce::String(c.axU) + " " + aU, juce::Rectangle<float>(386.0f, 31.0f, 260.0f, 18.0f), juce::Justification::right);
    g.drawText(juce::String(c.axD) + " " + aD, juce::Rectangle<float>(386.0f, 380.0f, 260.0f, 18.0f), juce::Justification::right);

    const Shade& ns = SHADES[active * 8 + nearest];
    const float dashes[2] = {3.0f, 3.0f};
    g.setColour(col.withAlpha(0.85f));
    g.drawDashedLine(juce::Line<float>(cur.x, cur.y, ns.x, ns.y), dashes, 2, 1.2f);

    for (int j = 0; j < 8; ++j) {
      const Shade& s = SHADES[active * 8 + j];
      const float dd = std::hypot(cur.x - s.x, cur.y - s.y);
      const float r = 8.0f + 7.0f * sm((130.0f - dd) / 110.0f);
      g.setColour(col.withAlpha(0.28f)); g.fillEllipse(s.x - r, s.y - r, 2 * r, 2 * r);
      g.setColour(col); g.drawEllipse(s.x - r, s.y - r, 2 * r, 2 * r, 1.2f);
      setF(g, 14.0f, j == nearest);
      g.setColour(juce::Colours::white.withAlpha(j == nearest ? 1.0f : 0.7f));
      g.drawText(s.name, juce::Rectangle<float>(s.x - 90.0f, s.y + 16.0f, 180.0f, 18.0f), juce::Justification::centred);
    }
    g.setColour(juce::Colours::white); g.fillEllipse(cur.x - 6.0f, cur.y - 6.0f, 12.0f, 12.0f);
    g.setColour(juce::Colour(0xff1a1530)); g.drawEllipse(cur.x - 6.0f, cur.y - 6.0f, 12.0f, 12.0f, 2.0f);
    if (alpha < 0.999f) g.endTransparencyLayer();
  }

  juce::StringArray chips() const {
    juce::StringArray c;
    if (proc.nost.load() >= 0.35f) c.add("borrowed chord");
    const int r = proc.res.load();
    c.add(r == RES_HOME ? "ends home" : (r == RES_HANG ? "ends hanging" : "ends twisted"));
    if (proc.air.load() >= 0.3f) c.add("anticipation");
    if (proc.tear.load() >= 0.4f) c.add("suspension");
    if (proc.reg.load() >= 0.35f) c.add("high topline");
    else if (proc.reg.load() <= -0.3f) c.add("low register");
    if (proc.strum.load()) c.add("soft roll");
    return c;
  }

  void paintInfo(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();
    auto info = b.removeFromBottom(INFO_H).reduced(2.0f, 0.0f);

    juce::String name, axes;
    if (active < 0) {
      name = "Pick a corner";
      axes = "Each corner is a core mood. Inside, it opens into shades.";
    } else {
      const Corner& c = CORNERS[active];
      name = juce::String(c.name);
      if (mode == InnerMode) {
        name += " > " + juce::String(SHADES[active * 8 + nearest].name);
        if (second >= 0 && w[second] >= 0.35f * w[nearest]) name += "  +  " + juce::String(SHADES[active * 8 + second].name);
        const int px = (int) std::lround(juce::jlimit(0.0f, 1.0f, (cur.x - 70.0f) / 520.0f) * 100.0f);
        const int py = (int) std::lround(juce::jlimit(0.0f, 1.0f, (375.0f - cur.y) / 270.0f) * 100.0f);
        const juce::String lr = U("â");
        axes = juce::String(c.axL) + " " + lr + " " + c.axR + ": " + juce::String(px) + "%" + dot()
             + c.axD + " " + lr + " " + c.axU + ": " + juce::String(py) + "%";
      } else axes = "Zooming in...";
    }
    const float textW = info.getWidth() - 120.0f;
    setF(g, 15.5f, true); g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.drawText(name, juce::Rectangle<float>(info.getX(), info.getY() + 4.0f, textW, 19.0f), juce::Justification::left);
    setF(g, 12.5f, false); g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawText(axes, juce::Rectangle<float>(info.getX(), info.getY() + 24.0f, info.getWidth(), 16.0f), juce::Justification::left);

    if (active >= 0 && mode == InnerMode) {
      setF(g, 11.0f, false);
      float x = info.getX();
      for (const auto& s : chips()) {
        const float cw = (float) s.length() * 6.0f + 18.0f;
        if (x + cw > info.getRight()) break;
        const auto pill = juce::Rectangle<float>(x, info.getY() + 41.0f, cw, 17.0f);
        g.setColour(juce::Colours::white.withAlpha(0.07f)); g.fillRoundedRectangle(pill, 8.5f);
        g.setColour(juce::Colours::white.withAlpha(0.22f)); g.drawRoundedRectangle(pill, 8.5f, 0.8f);
        g.setColour(juce::Colours::white.withAlpha(0.75f)); g.drawText(s, pill, juce::Justification::centred);
        x += cw + 6.0f;
      }
    }
  }

private:
  VibeSparkProcessor& proc;
  juce::TextButton back;
  Mode mode = MapMode;
  int active = -1, nearest = 0, second = -1;
  float t = 0.0f, animFrom = 0.0f, animTo = 0.0f, w[8] = {};
  double animStart = 0.0;
  juce::Point<float> cur{340.0f, 225.0f}, tg{340.0f, 225.0f}, sent{340.0f, 225.0f};
};

class VibeSparkEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
  explicit VibeSparkEditor(VibeSparkProcessor& p);
  ~VibeSparkEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;

private:
  void initCtl(Ctl& c, const char* nm, float v, std::function<void(float)> cb);
  static void placeCtl(Ctl& c, juce::Rectangle<int> a);
  void syncFromProc();
  void timerCallback() override;

  VibeSparkProcessor& proc;
  juce::ComboBox keyBox, voiBox, octBox, resBox;
  struct Ctl { juce::Label name; juce::Slider s; };
  Ctl cNost, cAir, cTear, cHuman;
  juce::TextButton genBtn, contBtn, preBtn, nobassBtn, strumBtn;
  juce::Label label; MoodPad pad; DragBox drag;
};
