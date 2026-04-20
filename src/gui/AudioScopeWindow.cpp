#include "AudioScopeWindow.h"
#include "core/AudioEngine.h"
#include "core/AppSettings.h"

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCloseEvent>
#include <QDateTime>
#include <cmath>

namespace AetherSDR {

// ── AudioScopeCanvas ────────────────────────────────────────────────

static constexpr int kSampleRate = AudioEngine::DEFAULT_SAMPLE_RATE;
static constexpr float kClipThreshold = 0.95f;
static constexpr int kClipFlashMs = 500;

AudioScopeCanvas::AudioScopeCanvas(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 120);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void AudioScopeCanvas::tick(AudioEngine* engine)
{
    if (m_frozen) return;

    // Number of samples for the full display width (5 divisions of m_timeBaseMs each)
    const int displaySamples = (m_timeBaseMs * 5 * kSampleRate) / 1000;
    // We need extra samples to search for a trigger point
    const int fetchSamples = std::min(displaySamples + 256,
                                      AudioEngine::kScopeTapSize);

    if (m_channel == Channel::Rx || m_channel == Channel::Both) {
        m_rxBuf.resize(fetchSamples);
        if (!engine->copyRecentScopeRxSamples(m_rxBuf.data(), fetchSamples))
            std::fill(m_rxBuf.begin(), m_rxBuf.end(), 0.0f);

        // Clip detection
        bool clipped = false;
        for (int i = 0; i < displaySamples && i < (int)m_rxBuf.size(); ++i) {
            if (std::fabs(m_rxBuf[i]) >= kClipThreshold) { clipped = true; break; }
        }
        if (clipped) {
            m_clipRx = true;
            m_clipRxExpiry = QDateTime::currentMSecsSinceEpoch() + kClipFlashMs;
        } else if (QDateTime::currentMSecsSinceEpoch() > m_clipRxExpiry) {
            m_clipRx = false;
        }
    }

    if (m_channel == Channel::Tx || m_channel == Channel::Both) {
        m_txBuf.resize(fetchSamples);
        if (!engine->copyRecentScopeTxSamples(m_txBuf.data(), fetchSamples))
            std::fill(m_txBuf.begin(), m_txBuf.end(), 0.0f);

        bool clipped = false;
        for (int i = 0; i < displaySamples && i < (int)m_txBuf.size(); ++i) {
            if (std::fabs(m_txBuf[i]) >= kClipThreshold) { clipped = true; break; }
        }
        if (clipped) {
            m_clipTx = true;
            m_clipTxExpiry = QDateTime::currentMSecsSinceEpoch() + kClipFlashMs;
        } else if (QDateTime::currentMSecsSinceEpoch() > m_clipTxExpiry) {
            m_clipTx = false;
        }
    }

    update();
}

int AudioScopeCanvas::findTrigger(const std::vector<float>& buf) const
{
    if (m_trigger == Trigger::FreeRun) return 0;

    const int displaySamples = (m_timeBaseMs * 5 * kSampleRate) / 1000;
    const int searchEnd = std::min((int)buf.size() - displaySamples, 256);
    if (searchEnd <= 1) return 0;

    for (int i = 1; i < searchEnd; ++i) {
        if (m_trigger == Trigger::Rising) {
            if (buf[i - 1] <= 0.0f && buf[i] > 0.0f) return i;
        } else { // Falling
            if (buf[i - 1] >= 0.0f && buf[i] < 0.0f) return i;
        }
    }
    return 0;  // no crossing found — fall back to start
}

void AudioScopeCanvas::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect r = rect();
    // Background
    p.fillRect(r, QColor(0x0f, 0x0f, 0x1a));

    const int displaySamples = (m_timeBaseMs * 5 * kSampleRate) / 1000;
    const float w = r.width();
    const float h = r.height();
    const float cy = h * 0.5f;

    // Clip-zone shading (top/bottom 5%)
    p.fillRect(QRectF(0, 0, w, h * 0.05f), QColor(80, 0, 0, 60));
    p.fillRect(QRectF(0, h * 0.95f, w, h * 0.05f), QColor(80, 0, 0, 60));

    // Grid
    QPen gridPen(QColor(60, 60, 80), 1, Qt::DotLine);
    p.setPen(gridPen);
    // Vertical: 5 divisions
    for (int i = 1; i < 5; ++i) {
        float x = w * i / 5.0f;
        p.drawLine(QPointF(x, 0), QPointF(x, h));
    }
    // Horizontal: 4 divisions
    for (int i = 1; i < 4; ++i) {
        float y = h * i / 4.0f;
        p.drawLine(QPointF(0, y), QPointF(w, y));
    }

    // Center line (0 V)
    QPen centerPen(QColor(80, 80, 100), 1, Qt::DashLine);
    p.setPen(centerPen);
    p.drawLine(QPointF(0, cy), QPointF(w, cy));

    // Time-base labels
    QFont labelFont = font();
    labelFont.setPixelSize(10);
    p.setFont(labelFont);
    p.setPen(QColor(100, 110, 130));
    for (int i = 0; i <= 5; ++i) {
        float x = w * i / 5.0f;
        QString lbl = QString("%1ms").arg(m_timeBaseMs * i);
        if (i == 0)
            p.drawText(QPointF(x + 2, h - 3), lbl);
        else if (i == 5)
            p.drawText(QPointF(x - 30, h - 3), lbl);
        else
            p.drawText(QPointF(x - 10, h - 3), lbl);
    }

    // Helper lambda: draw a waveform trace
    auto drawTrace = [&](const std::vector<float>& buf, const QColor& color) {
        if (buf.empty() || displaySamples <= 0) return;
        int offset = findTrigger(buf);

        QPainterPath path;
        const int count = std::min(displaySamples, (int)buf.size() - offset);
        if (count <= 0) return;

        for (int i = 0; i < count; ++i) {
            float x = (float)i / (float)(displaySamples - 1) * w;
            float sample = std::clamp(buf[offset + i], -1.0f, 1.0f);
            float y = cy - sample * (h * 0.45f);  // leave 5% margin for clip zone
            if (i == 0)
                path.moveTo(x, y);
            else
                path.lineTo(x, y);
        }

        p.setPen(QPen(color, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    };

    // Draw traces
    if (m_channel == Channel::Rx || m_channel == Channel::Both)
        drawTrace(m_rxBuf, QColor(0x00, 0xb4, 0xd8));  // cyan
    if (m_channel == Channel::Tx || m_channel == Channel::Both)
        drawTrace(m_txBuf, QColor(0xff, 0x8c, 0x00));  // orange

    // Legend (Both mode)
    if (m_channel == Channel::Both) {
        p.setFont(labelFont);
        p.setPen(QColor(0x00, 0xb4, 0xd8));
        p.drawText(QPointF(6, 14), "RX");
        p.setPen(QColor(0xff, 0x8c, 0x00));
        p.drawText(QPointF(28, 14), "TX");
    }

    // CLIP indicators
    QFont clipFont = font();
    clipFont.setPixelSize(12);
    clipFont.setBold(true);
    p.setFont(clipFont);
    p.setPen(QColor(0xff, 0x20, 0x20));
    if (m_clipRx && (m_channel == Channel::Rx || m_channel == Channel::Both))
        p.drawText(QPointF(w - 60, 16), "RX CLIP");
    if (m_clipTx && (m_channel == Channel::Tx || m_channel == Channel::Both))
        p.drawText(QPointF(w - 60, 30), "TX CLIP");
}

// ── AudioScopeWindow ────────────────────────────────────────────────

AudioScopeWindow::AudioScopeWindow(AudioEngine* engine, QWidget* parent)
    : QDialog(parent, Qt::Window)
    , m_audio(engine)
{
    setWindowTitle("Audio Scope");
    setAttribute(Qt::WA_DeleteOnClose, false);  // reuse across opens
    resize(600, 300);

    auto* vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(4, 4, 4, 4);
    vbox->setSpacing(4);

    // Toolbar
    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(6);

    toolbar->addWidget(new QLabel("Channel:"));
    m_channelCombo = new QComboBox;
    m_channelCombo->addItems({"RX", "TX", "Both"});
    toolbar->addWidget(m_channelCombo);

    toolbar->addWidget(new QLabel("Time/div:"));
    m_timeBaseCombo = new QComboBox;
    m_timeBaseCombo->addItems({"5 ms", "10 ms", "25 ms", "50 ms", "100 ms"});
    m_timeBaseCombo->setCurrentIndex(2);  // 25 ms default
    toolbar->addWidget(m_timeBaseCombo);

    toolbar->addWidget(new QLabel("Trigger:"));
    m_triggerCombo = new QComboBox;
    m_triggerCombo->addItems({"Free-run", "Rising edge", "Falling edge"});
    toolbar->addWidget(m_triggerCombo);

    m_freezeBtn = new QPushButton("Freeze");
    m_freezeBtn->setCheckable(true);
    toolbar->addWidget(m_freezeBtn);

    toolbar->addStretch();
    vbox->addLayout(toolbar);

    // Canvas
    m_canvas = new AudioScopeCanvas;
    vbox->addWidget(m_canvas, 1);

    // 30 fps repaint timer
    m_timer = new QTimer(this);
    m_timer->setInterval(33);  // ~30 fps
    connect(m_timer, &QTimer::timeout, this, [this]() {
        m_canvas->tick(m_audio);
    });

    // Wire controls
    connect(m_channelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        m_canvas->setChannel(static_cast<AudioScopeCanvas::Channel>(idx));
        saveSettings();
    });

    connect(m_timeBaseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        static const int tbValues[] = {5, 10, 25, 50, 100};
        if (idx >= 0 && idx < 5)
            m_canvas->setTimeBaseMs(tbValues[idx]);
        saveSettings();
    });

    connect(m_triggerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        m_canvas->setTrigger(static_cast<AudioScopeCanvas::Trigger>(idx));
        saveSettings();
    });

    connect(m_freezeBtn, &QPushButton::toggled, m_canvas, &AudioScopeCanvas::setFrozen);

    // Style — match app dark theme
    setStyleSheet(
        "QDialog { background: #0f0f1a; }"
        "QLabel { color: #8aa8c0; font-size: 11px; }"
        "QComboBox { background: #1a1a2e; color: #c8d8e8; border: 1px solid #2a3040; "
        "  padding: 2px 6px; font-size: 11px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: #1a1a2e; color: #c8d8e8; "
        "  selection-background-color: #2a4060; }"
        "QPushButton { background: #1a1a2e; color: #c8d8e8; border: 1px solid #2a3040; "
        "  padding: 3px 10px; font-size: 11px; }"
        "QPushButton:checked { background: #3a2020; color: #ff6060; border-color: #603030; }"
    );

    // Restore persisted settings
    restoreGeometry();
    auto& s = AppSettings::instance();
    int chIdx = s.value("AudioScopeChannel", "0").toInt();
    m_channelCombo->setCurrentIndex(qBound(0, chIdx, 2));

    int tbIdx = s.value("AudioScopeTimeBase", "2").toInt();
    m_timeBaseCombo->setCurrentIndex(qBound(0, tbIdx, 4));

    int trIdx = s.value("AudioScopeTrigger", "0").toInt();
    m_triggerCombo->setCurrentIndex(qBound(0, trIdx, 2));
}

AudioScopeWindow::~AudioScopeWindow() = default;

void AudioScopeWindow::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);
    m_audio->setScopeActive(true);
    m_timer->start();
}

void AudioScopeWindow::hideEvent(QHideEvent* ev)
{
    m_timer->stop();
    m_audio->setScopeActive(false);
    QDialog::hideEvent(ev);
}

void AudioScopeWindow::closeEvent(QCloseEvent* ev)
{
    saveGeometry();
    m_timer->stop();
    m_audio->setScopeActive(false);
    QDialog::closeEvent(ev);
}

void AudioScopeWindow::moveEvent(QMoveEvent* ev)
{
    QDialog::moveEvent(ev);
    saveGeometry();
}

void AudioScopeWindow::resizeEvent(QResizeEvent* ev)
{
    QDialog::resizeEvent(ev);
    saveGeometry();
}

void AudioScopeWindow::saveGeometry()
{
    auto& s = AppSettings::instance();
    s.setValue("AudioScopeGeometry",
              QString("%1,%2,%3,%4").arg(x()).arg(y()).arg(width()).arg(height()));
    s.save();
}

void AudioScopeWindow::restoreGeometry()
{
    auto& s = AppSettings::instance();
    QString geo = s.value("AudioScopeGeometry").toString();
    if (geo.isEmpty()) return;
    QStringList parts = geo.split(',');
    if (parts.size() == 4) {
        int gx = parts[0].toInt();
        int gy = parts[1].toInt();
        int gw = parts[2].toInt();
        int gh = parts[3].toInt();
        if (gw > 100 && gh > 60) {
            move(gx, gy);
            resize(gw, gh);
        }
    }
}

void AudioScopeWindow::saveSettings()
{
    auto& s = AppSettings::instance();
    s.setValue("AudioScopeChannel", QString::number(m_channelCombo->currentIndex()));
    s.setValue("AudioScopeTimeBase", QString::number(m_timeBaseCombo->currentIndex()));
    s.setValue("AudioScopeTrigger", QString::number(m_triggerCombo->currentIndex()));
    s.save();
}

} // namespace AetherSDR
