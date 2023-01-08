
#include <QtWidgets/QMessageBox>

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "AccountTab.hpp"
#include "LoginDialog.hpp"

#include "andromeda/backend/BackendImpl.hpp"
#include "andromeda-gui/BackendContext.hpp"

/*****************************************************/
MainWindow::MainWindow() : QMainWindow(),
    mQtUi(std::make_unique<Ui::MainWindow>()),
    mDebug("MainWindow",this)
{
    MDBG_INFO("()");

    mQtUi->setupUi(this);
}

/*****************************************************/
MainWindow::~MainWindow()
{
    MDBG_INFO("()");
}

/*****************************************************/
void MainWindow::show()
{
    MDBG_INFO("()");

    QMainWindow::show(); // base class
    activateWindow(); // bring to front

    if (GetCurrentTab() == nullptr) AddAccount();
}

/*****************************************************/
void MainWindow::closeEvent(QCloseEvent* event)
{
    MDBG_INFO("()");

    if (!event->spontaneous() || GetCurrentTab() == nullptr)
    {
        MDBG_INFO("... closing");
        QMainWindow::closeEvent(event);
    }
    else
    {
        MDBG_INFO("... hiding");
        event->ignore(); hide();
    }
}

/*****************************************************/
void MainWindow::AddAccount()
{
    MDBG_INFO("()");

    LoginDialog loginDialog(*this);
    if (loginDialog.exec())
    {
        std::unique_ptr<BackendContext> backendCtx { loginDialog.TakeBackend() };
        backendCtx->GetBackend().SetCacheManager(&mCacheManager);

        AccountTab* accountTab { new AccountTab(*this, std::move(backendCtx)) };

        mQtUi->tabAccounts->setCurrentIndex(
            mQtUi->tabAccounts->addTab(accountTab, accountTab->GetTabName().c_str()));

        mQtUi->actionMount_Storage->setEnabled(true);
        mQtUi->actionUnmount_Storage->setEnabled(true);
        mQtUi->actionBrowse_Storage->setEnabled(true);
        mQtUi->actionRemove_Account->setEnabled(true);
    }
}

/*****************************************************/
void MainWindow::RemoveAccount()
{
    MDBG_INFO("()");

    if (QMessageBox::question(this, "Remove Account", "Are you sure?") == QMessageBox::Yes)
        { MDBG_INFO("... confirmed"); }
    else return; // early return!

    AccountTab* accountTab { GetCurrentTab() };
    if (accountTab != nullptr)
    {
        int tabIndex { mQtUi->tabAccounts->indexOf(accountTab) };
        mQtUi->tabAccounts->removeTab(tabIndex); delete accountTab;
    }

    if (GetCurrentTab() == nullptr) // no accounts left
    {
        mQtUi->actionMount_Storage->setEnabled(false);
        mQtUi->actionUnmount_Storage->setEnabled(false);
        mQtUi->actionBrowse_Storage->setEnabled(false);
        mQtUi->actionRemove_Account->setEnabled(false);
    }
}

/*****************************************************/
AccountTab* MainWindow::GetCurrentTab()
{
    QWidget* widgetTab { mQtUi->tabAccounts->currentWidget() };
    return (widgetTab != nullptr) ? dynamic_cast<AccountTab*>(widgetTab) : nullptr;
}

/*****************************************************/
void MainWindow::MountCurrent()
{
    MDBG_INFO("()");

    AccountTab* accountTab { GetCurrentTab() };
    if (accountTab != nullptr) accountTab->Mount();
}

/*****************************************************/
void MainWindow::UnmountCurrent()
{
    MDBG_INFO("()");

    AccountTab* accountTab { GetCurrentTab() };
    if (accountTab != nullptr) accountTab->Unmount();
}

/*****************************************************/
void MainWindow::BrowseCurrent()
{
    MDBG_INFO("()");

    AccountTab* accountTab { GetCurrentTab() };
    if (accountTab != nullptr) accountTab->Browse();
}
