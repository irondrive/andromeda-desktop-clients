#ifndef A2GUI_MAINWINDOW_H
#define A2GUI_MAINWINDOW_H

#include <memory>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QMainWindow>

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/filesystem/filedata/CacheManager.hpp"

namespace Andromeda { 
    namespace Database { class SqliteDatabase; class ObjectDatabase; }
    namespace Filesystem { namespace Filedata { struct CacheOptions; } }
}

namespace AndromedaGui {
namespace Gui {

class AccountTab;

namespace Ui { class MainWindow; }

/** The main Andromeda GUI window */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit MainWindow(Andromeda::Filesystem::Filedata::CacheOptions& cacheOptions);

    ~MainWindow() override;
    DELETE_COPY(MainWindow)
    DELETE_MOVE(MainWindow)

    void closeEvent(QCloseEvent* event) override;

public slots:

    void show(); // override

    /** GUI action to add a new account */
    void AddAccount();

    /** GUI action to remove the current account */
    void RemoveAccount();

    /** GUI action to mount the current account's files */
    void MountCurrent();

    /** GUI action to unmount the current account's files */
    void UnmountCurrent();

    /** GUI action to browse the current account's files */
    void BrowseCurrent();

    /** Show the About popup window */
    void ShowAbout();

private:

    /** Returns the current AccountTab or nullptr if none */
    AccountTab* GetCurrentTab();

    mutable Andromeda::Debug mDebug;

    /** Global cache manager to apply to all mounts */
    Andromeda::Filesystem::Filedata::CacheManager mCacheManager;

    /** SqliteDatabase instance for the ObjectDatabase */
    std::unique_ptr<Andromeda::Database::SqliteDatabase> mSqlDatabase;
    /** ObjectDatabase instance for object storage */
    std::unique_ptr<Andromeda::Database::ObjectDatabase> mObjDatabase;

    std::unique_ptr<Ui::MainWindow> mQtUi;
};

} // namespace Gui
} // namespace AndromedaGui

#endif // A2GUI_MAINWINDOW_H
