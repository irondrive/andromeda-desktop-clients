#ifndef MOUNTSDIALOG_HPP
#define MOUNTSDIALOG_HPP

#include <QtWidgets/QDialog>

#include "andromeda/common.hpp"

namespace AndromedaGui {
namespace Gui {

namespace Ui { class MountsDialog; }

class MountsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MountsDialog(QWidget *parent = nullptr);
    
    ~MountsDialog() override;
    DELETE_COPY(MountsDialog)
    DELETE_MOVE(MountsDialog)

private:
    Ui::MountsDialog *ui;
};

} // namespace Gui
} // namespace AndromedaGui

#endif // MOUNTSDIALOG_HPP
