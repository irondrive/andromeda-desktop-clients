#ifndef A2GUI_LOGINDIALOG_HPP
#define A2GUI_LOGINDIALOG_HPP

#include <memory>
#include <QtWidgets/QDialog>
#include <QtWidgets/QWidget>

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"

namespace Ui { class LoginDialog; }

namespace AndromedaGui {
class BackendContext;

namespace QtGui {

/** The window for logging in (creating backend resources) */
class LoginDialog : public QDialog
{
    Q_OBJECT

public:

    explicit LoginDialog(QWidget& parent);

    ~LoginDialog() override;
    DELETE_COPY(LoginDialog)
    DELETE_MOVE(LoginDialog)

    /** 
     * Runs QDialog::exec() and creates a backend from the input username/password etc.
     * @param[out] backend backend gets created here
     * @return the return value of QDialog::exec()
     */
    int CreateBackend(std::unique_ptr<BackendContext>& backend);

public slots:

    void accept() override;

private:

    mutable Andromeda::Debug mDebug;

    /** The backend context created by the user */
    std::unique_ptr<BackendContext> mBackendContext;

    std::unique_ptr<Ui::LoginDialog> mQtUi;
};

} // namespace QtGui
} // namespace AndromedaGui

#endif // LOGINDIALOG_HPP
