#ifndef TOUCHAREA_H
#define TOUCHAREA_H

#include <QBitmap>
#include <QQuickWindow>

enum {
    NORMAL=0,
    FULLSCREEN
};

class TouchArea : public QObject
{
    Q_OBJECT
public:
    explicit TouchArea();
    ~TouchArea();

    Q_INVOKABLE void switchArea(int areaType);
    void setWindow(QQuickWindow* window);

public slots:
    void init();

private:
    QBitmap bitmapNormal, bitmapFullscreen;
    QQuickWindow* myWindow;
};

#endif // TOUCHAREA_H
