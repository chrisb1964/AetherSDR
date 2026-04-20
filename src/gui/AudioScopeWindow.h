#pragma once

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QTimer>
#include <vector>

namespace AetherSDR {

class AudioEngine;

// Real-time audio oscilloscope window for monitoring RX and TX audio
// waveforms (#1768).  Taps the existing post-DSP ring buffers in
// AudioEngine at ~30 fps and paints an antialiased polyline.
//
// Controls: Channel (RX / TX / Both), Time-base (5–100 ms/div),
// Trigger (Free-run / Rising / Falling), Freeze toggle.
class AudioScopeCanvas : public QWidget {
    Q_OBJECT

public:
    explicit AudioScopeCanvas(QWidget* parent = nullptr);

    enum class Channel { Rx, Tx, Both };
    enum class Trigger { FreeRun, Rising, Falling };

    void setChannel(Channel ch) { m_channel = ch; }
    void setTimeBaseMs(int ms) { m_timeBaseMs = ms; }
    void setTrigger(Trigger t) { m_trigger = t; }
    void setFrozen(bool on) { m_frozen = on; }

    // Called from the 30 fps timer — copies samples from AudioEngine
    // and triggers a repaint (unless frozen).
    void tick(AudioEngine* engine);

protected:
    void paintEvent(QPaintEvent* ev) override;

private:
    // Find a trigger point (zero-crossing) in the buffer.
    // Returns the index, or 0 if none found (free-run).
    int findTrigger(const std::vector<float>& buf) const;

    Channel m_channel{Channel::Rx};
    Trigger m_trigger{Trigger::FreeRun};
    int     m_timeBaseMs{25};
    bool    m_frozen{false};

    // Cached waveform data (one scope frame per path)
    std::vector<float> m_rxBuf;
    std::vector<float> m_txBuf;

    // Clip detection
    bool  m_clipRx{false};
    bool  m_clipTx{false};
    qint64 m_clipRxExpiry{0};
    qint64 m_clipTxExpiry{0};
};

class AudioScopeWindow : public QDialog {
    Q_OBJECT

public:
    explicit AudioScopeWindow(AudioEngine* engine, QWidget* parent = nullptr);
    ~AudioScopeWindow() override;

protected:
    void closeEvent(QCloseEvent* ev) override;
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;
    void moveEvent(QMoveEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;

private:
    void saveGeometry();
    void restoreGeometry();
    void saveSettings();

    AudioEngine*     m_audio;
    AudioScopeCanvas* m_canvas;
    QComboBox*       m_channelCombo;
    QComboBox*       m_timeBaseCombo;
    QComboBox*       m_triggerCombo;
    QPushButton*     m_freezeBtn;
    QTimer*          m_timer;
};

} // namespace AetherSDR
