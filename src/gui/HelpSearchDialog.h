#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QVector>

namespace AetherSDR {

class HelpSearchDialog : public QDialog {
    Q_OBJECT

public:
    explicit HelpSearchDialog(QWidget* parent = nullptr);

private:
    struct HelpTopic {
        QString title;
        QString resourcePath;
        QString markdown;
        QString plainText;
    };

    void buildUI();
    void loadTopics();
    void performSearch();
    void showTopic(int index);
    void highlightMatches(const QString& term);

    QLineEdit* m_searchBar = nullptr;
    QListWidget* m_resultsList = nullptr;
    QTextBrowser* m_contentPane = nullptr;
    QTimer* m_debounceTimer = nullptr;
    QVector<HelpTopic> m_topics;
    QString m_currentSearchTerm;
};

} // namespace AetherSDR
