#ifndef MPVRENDERER_H_
#define MPVRENDERER_H_

#include <MpvAbstractItem>
#include <MpvController>

class MpvObject : public MpvAbstractItem
{
    Q_OBJECT

public:
    explicit MpvObject(QQuickItem *parent = nullptr);

public slots:
    void command(const QVariant &params);
    void setProperty(const QString &name, const QVariant &value);
    QVariant getProperty(const QString &name);
    void observeProperty(const QString &name);

signals:
    void mpvEvent(const QString &ev, const QVariant &value);

private slots:
    void onPropertyChanged(const QString &property, const QVariant &value);
    void onFileLoaded();
    void onEndFile(const QString &reason);

private:
    void setupConnections();
};

#endif
