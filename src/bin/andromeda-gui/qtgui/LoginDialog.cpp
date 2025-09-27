#include <QtWidgets/QMessageBox>

#include "Utilities.hpp"
#include "LoginDialog.hpp"
#include "ui_LoginDialog.h"

#include "andromeda/SecureBuffer.hpp"
using Andromeda::SecureBuffer;
#include "andromeda/backend/BackendException.hpp"
using Andromeda::Backend::BackendException;
#include "andromeda-gui/BackendContext.hpp"

namespace AndromedaGui {
namespace QtGui {

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
        std::string password { mQtUi->lineEditPassword->text().toStdString() };
        const std::string twofactor { mQtUi->lineEditTwoFactor->text().toStdString() };

        const SecureBuffer passbuf { SecureBuffer::Insecure_FromBuf(password.data(), password.size()) };
        mBackendContext = std::make_unique<BackendContext>(apiurl, username, passbuf, twofactor);

        // best effort zeroize of insecure buffers
        mQtUi->lineEditPassword->text().fill('\0');
        password.resize(password.capacity());
        std::fill_n(password.data(), password.size(), '\0');
    }
    catch (const BackendException& ex)
    {
        MDBG_ERROR("... " << ex.what());
        Utilities::criticalBox(this, "Login Error", "Failed to login to the server.", ex);
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

} // namespace QtGui
} // namespace AndromedaGui
