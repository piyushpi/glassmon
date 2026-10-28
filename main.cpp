// main.cpp
#include <QWindow>
#include <QPainterPath>
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QMouseEvent>
#include <QProcess>
#include <QMenu>
#include <QContextMenuEvent>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QRectF>
#include <deque>

class HardwareWidget : public QWidget {
public:
    HardwareWidget() {
        setWindowFlags(
            Qt::FramelessWindowHint |
            Qt::Tool |
            Qt::WindowStaysOnTopHint
        );

        setAttribute(Qt::WA_TranslucentBackground);

        resize(360, 290);

        pollTimer = new QTimer(this);

        connect(pollTimer, &QTimer::timeout, this, [this]() {
            updateSensors();
            update();
        });

        pollTimer->start(1000);

        readCpuStats(prevIdle, prevTotal);

        updateSensors();

        setupAutostart();
    }

private:
    void setupAutostart() {
        QDir autostartDir(QDir::homePath() + "/.config/autostart");

        if (!autostartDir.exists())
            autostartDir.mkpath(".");

        QString desktopFile = autostartDir.filePath("hardware-monitor.desktop");

        QFile file(desktopFile);

        if (file.exists())
            return;

        if (file.open(QIODevice::WriteOnly)) {
            QTextStream ts(&file);

            ts << "[Desktop Entry]\n"
               << "Type=Application\n"
               << "Name=Hardware Monitor\n"
               << "Exec=" << QCoreApplication::applicationFilePath() << "\n"
               << "Hidden=false\n"
               << "NoDisplay=false\n"
               << "X-GNOME-Autostart-enabled=true\n";
        }
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);

        p.setRenderHint(QPainter::Antialiasing);

        QRectF bg = rect().adjusted(4,4,-4,-4);

        QPainterPath path;
        path.addRoundedRect(bg, 24, 24);

        p.fillPath(path, QColor(18,18,22,230));

        p.setPen(QPen(QColor(255,255,255,18),1));
        p.drawPath(path);

        drawHeader(p);

        drawSection(
            p,
            24,
            70,
            "CPU",
            cpuTemp,
            cpuUsage,
            cpuFan,
            cpuHistory
        );

        drawSection(
            p,
            24,
            180,
            "GPU",
            gpuTemp,
            gpuUsage,
            gpuFan,
            gpuHistory
        );
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            if (windowHandle()) {
                windowHandle()->startSystemMove();
            }
            event->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (dragging &&
            (event->buttons() & Qt::LeftButton)) {

            QPoint newPos =
                event->globalPosition().toPoint()
                - dragPosition;

            move(newPos);

            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        dragging = false;
        event->accept();
    }

    void contextMenuEvent(QContextMenuEvent* event) override {
        QMenu menu(this);
        menu.setStyleSheet(
            "QMenu { background: #1c1c22; border: 1px solid #333; border-radius: 8px; padding: 4px; }"
            "QMenu::item { padding: 6px 24px; border-radius: 4px; color: #ddd; }"
            "QMenu::item:selected { background: #2a2a35; }"
        );
        QAction* topAction = menu.addAction("Always on Top");
        topAction->setCheckable(true);
        topAction->setChecked(alwaysOnTop);
        connect(topAction, &QAction::toggled, this, [this](bool on) {
            alwaysOnTop = on;
            if (on)
                setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
            else
                setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
            setAttribute(Qt::WA_TranslucentBackground);
            show();
        });
        menu.addSeparator();
        QAction* exitAction = menu.addAction("Exit");
        connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
        menu.exec(event->globalPos());
    }

private:
    QTimer* pollTimer;

    bool dragging = false;
    bool alwaysOnTop = true;
    QPoint dragPosition;

    float cpuTemp = 0;
    float gpuTemp = 0;

    float cpuUsage = 0;
    float gpuUsage = 0;

    int cpuFan = 0;
    int gpuFan = 0;

    long long prevIdle = 0;
    long long prevTotal = 0;

    std::deque<float> cpuHistory;
    std::deque<float> gpuHistory;

    QString run(const QString& cmd) {
        QProcess p;

        p.start("sh", {"-c", cmd});
        p.waitForFinished(1000);

        return p.readAllStandardOutput().trimmed();
    }

    void drawHeader(QPainter& p) {
        QFont title("Inter", 11);
        title.setBold(true);

        p.setFont(title);
        p.setPen(QColor(240,240,240));

        p.drawText(24, 34, "glassmon");
    }

    void drawSection(
        QPainter& p,
        int x,
        int y,
        const QString& label,
        float temp,
        float usage,
        int fan,
        const std::deque<float>& history
    ) {
        QFont small("Inter", 10);
        p.setFont(small);

        p.setPen(QColor(150,150,160));

        p.drawText(x, y, label);

        QFont big("Inter", 28);
        big.setBold(true);

        p.setFont(big);

        p.setPen(QColor(245,245,245));

        p.drawText(
            x,
            y + 42,
            QString::number(temp, 'f', 1) + "°C"
        );

        p.setFont(QFont("Inter", 10));

        p.setPen(QColor(130,130,140));

        p.drawText(
            x,
            y + 68,
            QString("Usage %1%").arg(
                QString::number(usage, 'f', 0)
            )
        );

        p.drawText(
            x,
            y + 88,
            QString("Fan %1 RPM").arg(fan)
        );

        drawGraph(
            p,
            x + 170,
            y + 8,
            140,
            60,
            history
        );
    }

    void drawGraph(
        QPainter& p,
        int x,
        int y,
        int w,
        int h,
        const std::deque<float>& values
    ) {
        if (values.size() < 2)
            return;

        QPainterPath path;

        for (size_t i = 0; i < values.size(); ++i) {
            float nx = float(i) / float(values.size() - 1);

            float ny = (values[i] - 20.f) / 80.f;

            float px = x + nx * w;
            float py = y + h - ny * h;

            if (i == 0)
                path.moveTo(px, py);
            else
                path.lineTo(px, py);
        }

        p.setPen(QPen(QColor(120,180,255), 2));
        p.drawPath(path);
    }

    void push(std::deque<float>& d, float v) {
        d.push_back(v);

        while (d.size() > 60)
            d.pop_front();
    }

    float readCpuTemp() {
        QString out = run(
            "sensors | grep 'Package id 0:' | awk '{print $4}' | tr -d '+°C'"
        );

        bool ok;
        float t = out.toFloat(&ok);

        if (ok)
            return t;

        out = run(
            "cat /sys/class/thermal/thermal_zone0/temp"
        );

        t = out.toFloat(&ok);

        if (ok)
            return t / 1000.f;

        return 0;
    }

    int readFanSpeed() {
        QDir dir("/sys/class/hwmon");

        QStringList hwmons =
            dir.entryList(
                QDir::Dirs | QDir::NoDotAndDotDot
            );

        for (const QString& hw : hwmons) {
            QString hwPath =
                "/sys/class/hwmon/" + hw;

            for (int i = 1; i <= 10; ++i) {
                QString path =
                    hwPath + "/fan" + QString::number(i) + "_input";

                QFile file(path);

                if (file.open(QIODevice::ReadOnly)) {
                    QString value =
                        QTextStream(&file).readAll().trimmed();

                    bool ok;
                    int rpm = value.toInt(&ok);

                    if (ok && rpm > 0)
                        return rpm;
                }
            }
        }

        QString out = run(
            "sensors 2>/dev/null | grep -i 'fan' | awk '{print $2}' | tr -d '+' | head -1"
        );

        if (!out.isEmpty()) {
            bool ok;
            int rpm = out.toInt(&ok);
            if (ok && rpm > 0)
                return rpm;
        }

        return 0;
    }

    float readCpuUsage() {
        QFile file("/proc/stat");

        if (!file.open(QIODevice::ReadOnly))
            return 0;

        QTextStream in(&file);

        QString line = in.readLine();

        QStringList values =
            line.split(" ", Qt::SkipEmptyParts);

        if (values.size() < 5)
            return 0;

        long long user = values[1].toLongLong();
        long long nice = values[2].toLongLong();
        long long system = values[3].toLongLong();
        long long idle = values[4].toLongLong();

        long long total =
            user + nice + system + idle;

        long long totalDiff = total - prevTotal;
        long long idleDiff = idle - prevIdle;

        prevTotal = total;
        prevIdle = idle;

        if (totalDiff == 0)
            return 0;

        return 100.f *
               (1.f - float(idleDiff) / totalDiff);
    }

    void readCpuStats(
        long long& idle,
        long long& total
    ) {
        QFile file("/proc/stat");

        if (!file.open(QIODevice::ReadOnly))
            return;

        QTextStream in(&file);

        QString line = in.readLine();

        QStringList values =
            line.split(" ", Qt::SkipEmptyParts);

        long long user = values[1].toLongLong();
        long long nice = values[2].toLongLong();
        long long system = values[3].toLongLong();

        idle = values[4].toLongLong();

        total = user + nice + system + idle;
    }

    float readGpuTemp() {
        QString out = run(
            "nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits 2>/dev/null"
        );

        bool ok;
        float t = out.toFloat(&ok);

        if (ok)
            return t;

        return 0;
    }

    float readGpuUsage() {
        QString out = run(
            "nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits 2>/dev/null"
        );

        bool ok;
        float u = out.toFloat(&ok);

        if (ok)
            return u;

        return 0;
    }

    int readGpuFan() {
        QString out = run(
            "nvidia-smi --query-gpu=fan.speed --format=csv,noheader,nounits 2>/dev/null"
        );

        bool ok;
        int rpm = out.replace("%","").toInt(&ok);

        if (ok)
            return rpm;

        return 0;
    }

    void updateSensors() {
        cpuTemp = readCpuTemp();
        cpuUsage = readCpuUsage();
        cpuFan = readFanSpeed();

        gpuTemp = readGpuTemp();
        gpuUsage = readGpuUsage();
        gpuFan = readGpuFan();

        push(cpuHistory, cpuTemp);
        push(gpuHistory, gpuTemp);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    HardwareWidget widget;

    widget.show();

    return app.exec();
}