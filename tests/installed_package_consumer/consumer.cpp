// A reachable registration call forces the installed archive and its Qt
// dependencies into the link, even with Release optimization enabled.
#include "vnm_font_namespace.h"

#include <QGuiApplication>
#include <cstdio>

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    const auto registered = vnm_fonts::register_shipped_font(
        vnm_fonts::Shipped_font::ROBOTO_CONDENSED_REGULAR);
    if (!registered.is_valid()) {
        std::fprintf(stderr, "%s\n", qPrintable(registered.error));
        return 1;
    }
    return 0;
}
