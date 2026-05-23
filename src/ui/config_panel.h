#ifndef CONFIG_PANEL_H
#define CONFIG_PANEL_H
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QVariantMap>
class ConfigPanel : public QWidget {
    Q_OBJECT
public:
    explicit ConfigPanel(QWidget* parent = nullptr);
    ~ConfigPanel() override;
    QString selectedOutputFormat() const;
    QString outputDirectory() const;
    QVariantMap conversionParams() const;
public slots:
    void loadSettings();
    void saveSettings();
private slots:
    void onBrowseOutputDir();
    void onOutputFormatChanged(int index);
private:
    void setupUI();
    void setupConnections();
    void applyStyleSheet();
    void updateOutputFormats();
    QComboBox* m_outputFormatCombo;
    QLineEdit* m_outputDirEdit;
    QPushButton* m_browseOutputBtn;
};
#endif // CONFIG_PANEL_H
