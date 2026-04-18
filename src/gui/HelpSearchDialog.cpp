#include "HelpSearchDialog.h"

#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

namespace AetherSDR {

// The help topics bundled in resources.qrc, in menu order.
static const struct { const char* title; const char* resource; } kHelpFiles[] = {
    { "Getting Started",                  ":/help/getting-started.md" },
    { "AetherSDR Help",                   ":/help/aethersdr-help.md" },
    { "Understanding Noise Cancellation", ":/help/understanding-noise-cancellation.md" },
    { "Configuring AetherSDR Controls",   ":/help/configuring-aethersdr-controls.md" },
    { "Configuring Data Modes",           ":/help/understanding-data-modes.md" },
    { "Contributing to AetherSDR",        ":/help/contributing-to-aethersdr.md" },
};

HelpSearchDialog::HelpSearchDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle("Search Help");
    loadTopics();
    buildUI();
}

void HelpSearchDialog::loadTopics()
{
    for (const auto& entry : kHelpFiles) {
        HelpTopic topic;
        topic.title = QString::fromUtf8(entry.title);
        topic.resourcePath = QString::fromUtf8(entry.resource);

        QFile file(topic.resourcePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            topic.markdown = QString::fromUtf8(file.readAll());
        else
            topic.markdown = QStringLiteral("*(Help file not available)*");

        // Build a plain-text version for search matching.
        // Strip Markdown formatting characters for cleaner matching.
        topic.plainText = topic.markdown;
        topic.plainText.remove(QChar('#'));
        topic.plainText.remove(QChar('*'));
        topic.plainText.remove(QChar('`'));

        m_topics.append(topic);
    }
}

void HelpSearchDialog::buildUI()
{
    resize(900, 640);
    setMinimumSize(640, 420);

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // --- Header ---
    auto* header = new QWidget(this);
    header->setStyleSheet("background: #0a0a14;");
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(20, 14, 20, 10);
    headerLayout->setSpacing(6);

    auto* eyebrow = new QLabel("AETHERSDR HELP", header);
    eyebrow->setStyleSheet("color: #00b4d8; font-size: 11px; letter-spacing: 2px;");
    headerLayout->addWidget(eyebrow);

    // Search bar
    m_searchBar = new QLineEdit(header);
    m_searchBar->setPlaceholderText("Search help\u2026");
    m_searchBar->setClearButtonEnabled(true);
    m_searchBar->setStyleSheet(
        "QLineEdit {"
        "  background: #152230;"
        "  color: #d8e4ee;"
        "  border: 1px solid #304050;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  font-size: 14px;"
        "}"
        "QLineEdit:focus { border-color: #00b4d8; }");
    headerLayout->addWidget(m_searchBar);

    outerLayout->addWidget(header);

    // --- Separator ---
    auto* separator = new QWidget(this);
    separator->setFixedHeight(1);
    separator->setStyleSheet("background: #203040;");
    outerLayout->addWidget(separator);

    // --- Splitter: results list + content pane ---
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setStyleSheet("QSplitter::handle { background: #203040; width: 1px; }");

    m_resultsList = new QListWidget(splitter);
    m_resultsList->setStyleSheet(
        "QListWidget {"
        "  background: #0f0f1a;"
        "  color: #c8d8e8;"
        "  border: none;"
        "  font-size: 13px;"
        "  outline: none;"
        "}"
        "QListWidget::item {"
        "  padding: 8px 12px;"
        "  border-bottom: 1px solid #18202c;"
        "}"
        "QListWidget::item:selected {"
        "  background: #1a2a3a;"
        "  color: #00b4d8;"
        "}"
        "QListWidget::item:hover {"
        "  background: #141e2a;"
        "}");

    m_contentPane = new QTextBrowser(splitter);
    m_contentPane->setReadOnly(true);
    m_contentPane->setOpenExternalLinks(true);
    m_contentPane->document()->setDocumentMargin(18);
    m_contentPane->document()->setDefaultStyleSheet(
        "body { color: #c8d8e8; font-size: 12px; font-family: sans-serif; }"
        "h1 { color: #00b4d8; font-size: 22px; font-weight: 700; margin: 0 0 16px 0; }"
        "h2 { color: #c8d8e8; font-size: 18px; font-weight: 700; margin: 18px 0 8px 0; }"
        "h3 { color: #8898a8; font-size: 14px; font-weight: 700; margin: 14px 0 6px 0; }"
        "h4 { color: #8898a8; font-size: 12px; font-weight: 700; margin: 12px 0 4px 0; }"
        "p { color: #c8d8e8; font-size: 12px; line-height: 1.5; margin: 0 0 10px 0; }"
        "ul, ol { margin: 0 0 12px 20px; }"
        "li { color: #c8d8e8; font-size: 12px; line-height: 1.55; margin: 3px 0; }"
        "ol li::marker { color: #00b4d8; font-weight: 700; }"
        "ul li::marker { color: #40c060; }"
        "strong { color: #dfeaf3; font-weight: 700; }"
        "em { color: #8898a8; font-style: italic; }"
        "code { color: #d8e4ee; background-color: #152230; }"
        "a { color: #00b4d8; text-decoration: none; }");
    m_contentPane->setStyleSheet(
        "QTextBrowser {"
        "  background: #0f0f1a;"
        "  color: #d8e4ee;"
        "  border: none;"
        "  padding: 10px;"
        "  selection-background-color: #245a7a;"
        "  font-size: 14px;"
        "}"
        "QScrollBar:vertical {"
        "  background: #0a0a14; width: 10px; margin: 8px 2px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #304050; min-height: 28px; border-radius: 5px;"
        "}"
        "QScrollBar::handle:vertical:hover { background: #3f5870; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar:horizontal {"
        "  background: #0a0a14; height: 10px; margin: 2px 8px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: #304050; min-width: 28px; border-radius: 5px;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }");

    splitter->addWidget(m_resultsList);
    splitter->addWidget(m_contentPane);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);

    outerLayout->addWidget(splitter, 1);

    // --- Footer ---
    auto* footer = new QWidget(this);
    footer->setStyleSheet("background: #0a0a14;");
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(16, 10, 16, 14);

    auto* hint = new QLabel("Type a keyword to search across all help topics.", footer);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #7f93a7; font-size: 11px;");
    footerLayout->addWidget(hint, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, footer);
    auto* closeButton = buttons->button(QDialogButtonBox::Close);
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setStyleSheet(
        "QPushButton {"
        "  background: #00b4d8; color: #0f0f1a; font-weight: 700;"
        "  border: none; border-radius: 16px;"
        "  min-width: 96px; min-height: 32px; padding: 0 18px;"
        "}"
        "QPushButton:hover { background: #18c8ea; }");
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    footerLayout->addWidget(buttons, 0, Qt::AlignRight);

    outerLayout->addWidget(footer);

    setStyleSheet("HelpSearchDialog { background: #0f0f1a; }");

    // --- Populate the results list with all topics ---
    for (int i = 0; i < m_topics.size(); ++i) {
        auto* item = new QListWidgetItem(m_topics[i].title);
        item->setData(Qt::UserRole, i);
        m_resultsList->addItem(item);
    }

    // Show the first topic by default.
    if (!m_topics.isEmpty()) {
        m_resultsList->setCurrentRow(0);
        showTopic(0);
    }

    // --- Wiring ---
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(150);
    connect(m_debounceTimer, &QTimer::timeout, this, &HelpSearchDialog::performSearch);

    connect(m_searchBar, &QLineEdit::textChanged, this, [this]() {
        m_debounceTimer->start();
    });

    connect(m_resultsList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0) return;
        // Map the visible list row back to the topic index stored in item data.
        int topicIndex = m_resultsList->item(row)->data(Qt::UserRole).toInt();
        showTopic(topicIndex);
    });

    m_searchBar->setFocus();
}

void HelpSearchDialog::performSearch()
{
    const QString term = m_searchBar->text().trimmed();
    m_currentSearchTerm = term;

    m_resultsList->clear();

    if (term.isEmpty()) {
        // Show all topics.
        for (int i = 0; i < m_topics.size(); ++i) {
            auto* item = new QListWidgetItem(m_topics[i].title);
            item->setData(Qt::UserRole, i);
            m_resultsList->addItem(item);
        }
        if (!m_topics.isEmpty()) {
            m_resultsList->setCurrentRow(0);
            showTopic(0);
        }
        return;
    }

    // Filter: case-insensitive substring match on title + plain text.
    for (int i = 0; i < m_topics.size(); ++i) {
        const auto& t = m_topics[i];
        if (t.title.contains(term, Qt::CaseInsensitive) ||
            t.plainText.contains(term, Qt::CaseInsensitive)) {
            auto* item = new QListWidgetItem(t.title);
            item->setData(Qt::UserRole, i);
            m_resultsList->addItem(item);
        }
    }

    if (m_resultsList->count() > 0) {
        m_resultsList->setCurrentRow(0);
    } else {
        m_contentPane->setMarkdown(
            QStringLiteral("# No results\n\nNo help topics match **%1**.").arg(term.toHtmlEscaped()));
    }
}

void HelpSearchDialog::showTopic(int index)
{
    if (index < 0 || index >= m_topics.size()) return;

    m_contentPane->setMarkdown(m_topics[index].markdown);

    if (!m_currentSearchTerm.isEmpty())
        highlightMatches(m_currentSearchTerm);
}

void HelpSearchDialog::highlightMatches(const QString& term)
{
    // Move cursor to start so find() searches the entire document.
    auto cursor = m_contentPane->textCursor();
    cursor.movePosition(QTextCursor::Start);
    m_contentPane->setTextCursor(cursor);

    // Use QTextBrowser::find() in a loop to highlight all occurrences.
    // Each successful find() selects the match, which the selection color shows.
    // We build extra selections for persistent highlighting.
    QList<QTextEdit::ExtraSelection> selections;

    QTextCharFormat fmt;
    fmt.setBackground(QColor("#3a5a2a"));
    fmt.setForeground(QColor("#e0ffe0"));

    while (m_contentPane->find(term)) {
        QTextEdit::ExtraSelection sel;
        sel.cursor = m_contentPane->textCursor();
        sel.format = fmt;
        selections.append(sel);
    }

    m_contentPane->setExtraSelections(selections);

    // Scroll to the first match.
    if (!selections.isEmpty()) {
        m_contentPane->setTextCursor(selections.first().cursor);
        m_contentPane->ensureCursorVisible();
    }
}

} // namespace AetherSDR
