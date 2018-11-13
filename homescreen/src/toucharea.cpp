#include "toucharea.h"
#include "hmi-debug.h"

TouchArea::TouchArea()
{
}

TouchArea::~TouchArea()
{

}

void TouchArea::setWindow(QQuickWindow *window)
{
    myWindow = window;
}

void TouchArea::init()
{
    bitmapNormal = QPixmap(":/images/AGL_HMI_Normal_Background.png").createHeuristicMask();
    bitmapFullscreen = QPixmap(":/images/AGL_HMI_Full_Background.png").createHeuristicMask();
    myWindow->setMask(QRegion(bitmapNormal));
}

void TouchArea::switchArea(int areaType)
{
    if(areaType == NORMAL) {
        myWindow->setMask(QRegion(bitmapNormal));
        HMI_DEBUG("HomeScreen","TouchArea switchArea: %d.", areaType);
    } else if (areaType == FULLSCREEN) {
        HMI_DEBUG("HomeScreen","TouchArea switchArea: %d.", areaType);
        myWindow->setMask(QRegion(bitmapFullscreen));
    }
}


