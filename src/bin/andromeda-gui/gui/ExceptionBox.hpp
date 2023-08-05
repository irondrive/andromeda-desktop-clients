#ifndef A2GUI_EXCEPTIONBOX_H
#define A2GUI_EXCEPTIONBOX_H

#include <sstream>

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QWidget>

#include "andromeda/BaseException.hpp"

namespace AndromedaGui
{
/** Helper for QMessageBoxes with exceptions */
struct ExceptionBox
{
    ExceptionBox() = delete; // static only

    /** See QMessageBox::critical */
    static inline void critical(QWidget* parent, const std::string& title, const std::string& msg, const Andromeda::BaseException& ex)
    {
        std::stringstream str; str << msg << std::endl << std::endl; str << ex.what();
        QMessageBox::critical(parent, title.c_str(), str.str().c_str());
    }

    /** See QMessageBox::warning */
    static inline void warning(QWidget* parent, const std::string& title, const std::string& msg, const Andromeda::BaseException& ex)
    {
        std::stringstream str; str << msg << std::endl << std::endl; str << ex.what();
        QMessageBox::warning(parent, title.c_str(), str.str().c_str());
    }
};

} // namespace AndromedaGui

#endif // A2GUI_EXCEPTIONBOX_H_