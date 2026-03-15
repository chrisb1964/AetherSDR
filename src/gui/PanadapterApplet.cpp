#include "PanadapterApplet.h"
#include "SpectrumWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace AetherSDR {

PanadapterApplet::PanadapterApplet(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ── Title bar (16px gradient, matching applet style) ─────────────────
    auto* titleBar = new QWidget;
    titleBar->setFixedHeight(16);
    titleBar->setStyleSheet(
        "QWidget { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "stop:0 #3a4a5a, stop:0.5 #2a3a4a, stop:1 #1a2a38); "
        "border-bottom: 1px solid #0a1a28; }");

    auto* barLayout = new QHBoxLayout(titleBar);
    barLayout->setContentsMargins(6, 1, 4, 1);
    barLayout->setSpacing(2);

    m_titleLabel = new QLabel("Slice A");
    m_titleLabel->setStyleSheet("QLabel { background: transparent; color: #8aa8c0; "
                                "font-size: 10px; font-weight: bold; }");
    barLayout->addWidget(m_titleLabel);
    barLayout->addStretch();

    // Placeholder window control buttons (non-functional for now)
    const QString btnStyle = QStringLiteral(
        "QPushButton { background: transparent; color: #6a8090; "
        "border: none; font-size: 9px; padding: 0; }"
        "QPushButton:hover { color: #c8d8e8; }");

    for (const char* icon : {"_", "\u25A1", "\u00D7"}) {
        auto* btn = new QPushButton(icon);
        btn->setFixedSize(14, 14);
        btn->setStyleSheet(btnStyle);
        barLayout->addWidget(btn);
    }

    layout->addWidget(titleBar);

    // ── Spectrum widget (FFT + waterfall) ────────────────────────────────
    m_spectrum = new SpectrumWidget(this);
    layout->addWidget(m_spectrum, 1);
}

void PanadapterApplet::setSliceId(int id)
{
    const char letters[] = "ABCD";
    const char letter = (id >= 0 && id < 4) ? letters[id] : '?';
    m_titleLabel->setText(QString("Slice %1").arg(letter));
}

} // namespace AetherSDR
