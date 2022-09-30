#include <QThread>

class WorkerThread : public QThread
{
	Q_OBJECT
	void run() override;
};
