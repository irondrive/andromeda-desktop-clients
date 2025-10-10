#include <QtWidgets/QMessageBox>

#include "Utilities.hpp"
#include "LoginDialog.hpp"
#include "ui_LoginDialog.h"

#include "andromeda/Crypto.hpp"
using Andromeda::Crypto;
#include "andromeda/SecureBuffer.hpp"
using Andromeda::SecureBuffer;
#include "andromeda/StringUtil.hpp"
using Andromeda::StringUtil;
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
        std::string e2ee_recovery { mQtUi->lineEditE2eeRecovery->text().toStdString() };
        const std::string twofactor { mQtUi->lineEditTwoFactor->text().toStdString() };

        const SecureBuffer passbuf { SecureBuffer::Insecure_FromStr(password) };
        const SecureBuffer e2eebuf { SecureBuffer::Insecure_FromStr(e2ee_recovery) };
        mBackendContext = std::make_unique<BackendContext>(apiurl, username, passbuf, e2eebuf, twofactor);

        // best effort zeroize of insecure buffers
        mQtUi->lineEditPassword->text().fill('\0');
        StringUtil::Zeroize(password);
        mQtUi->lineEditE2eeRecovery->text().fill('\0');
        StringUtil::Zeroize(e2ee_recovery);
    }
    catch (const Crypto::Exception& ex)
    {
        MDBG_ERROR("... " << ex.what());
        Utilities::criticalBox(this, "Crypto Error", "Failed to initialize e2ee (wrong key?)", ex);
        return; // no accept()
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
