#include <QtWidgets/QMessageBox>

#include "ExceptionBox.hpp"
#include "LoginDialog.hpp"
#include "ui_LoginDialog.h"

#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;
#include "andromeda-gui/BackendContext.hpp"

namespace AndromedaGui {
namespace Gui {

/*****************************************************/
LoginDialog::LoginDialog(QWidget& parent) : QDialog(&parent),
    mDebug(__func__,this),
    mQtUi(std::make_unique<Ui::LoginDialog>())
{
    MDBG_INFO("()");

    mQtUi->setupUi(this);
}

/*****************************************************/
LoginDialog::~LoginDialog() 
{ 
    MDBG_INFO("()");
}

/*****************************************************/
void LoginDialog::accept()
{
    MDBG_INFO("()");

    std::string apiurl { mQtUi->lineEditServerUrl->text().toStdString() };
    std::string username { mQtUi->lineEditUsername->text().toStdString() };

    MDBG_INFO("... apiurl:(" << apiurl << ") username:(" << username << ")");

    try
    {
        const std::string password { mQtUi->lineEditPassword->text().toStdString() };
        const std::string twofactor { mQtUi->lineEditTwoFactor->text().toStdString() };
        
        mBackendContext = std::make_unique<BackendContext>(apiurl, username, password, twofactor);
    }
    catch (const BaseException& ex)
    {
        MDBG_ERROR("... " << ex.what());
        ExceptionBox::critical(this, "Login Error", "Failed to login to the server.", ex);
        return; // no accept()
    }

    QDialog::accept();
}

/*****************************************************/
int LoginDialog::CreateBackend(std::unique_ptr<BackendContext>& backend)
{
    const int retval { QDialog::exec() };
    if (retval)
    {
        backend = std::move(mBackendContext);
        mBackendContext.reset();
    }
    return retval;
}

} // namespace Gui
} // namespace AndromedaGui
