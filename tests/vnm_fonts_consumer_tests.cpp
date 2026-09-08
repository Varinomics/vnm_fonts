// Exercises the library the way a consumer does: linked through vnm::fonts,
// with no source of the library in this target, no font bytes handled here and
// no family override written down here.
//
//   1. The resource initialiser makes ":/vnm_fonts/..." readable from a target
//      that merely links the static library. Without it the linker is free to
//      discard the generated resource initialiser and the fonts vanish.
//   2. Every shipped font registers by id and comes back with a marked family.
//   3. Registering the same font twice returns the first registration rather
//      than registering the bytes again.
//   4. The Font Awesome 7 Free pair lands in two families, which is the case
//      that depends on the override the library resolves internally. A
//      consumer that had to supply that itself could forget it.
//
// Failures print to stderr and the process exits non-zero.

#include "vnm_font_namespace.h"

#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QSet>
#include <QString>
#include <cstdio>
#include <vector>

namespace {

int s_failures = 0;

bool check(bool condition, const QString& message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL %s\n", qPrintable(message));
        ++s_failures;
    }
    return condition;
}

std::vector<vnm_fonts::Shipped_font> every_shipped_font()
{
    using vnm_fonts::Shipped_font;
    return {
        Shipped_font::ROBOTO_CONDENSED_REGULAR,
        Shipped_font::ROBOTO_CONDENSED_LIGHT,
        Shipped_font::FONT_AWESOME_4,
        Shipped_font::FONT_AWESOME_7_BRANDS,
        Shipped_font::FONT_AWESOME_7_FREE_REGULAR,
        Shipped_font::FONT_AWESOME_7_FREE_SOLID,
        Shipped_font::NOTO_SANS_SYMBOLS_2,
        Shipped_font::JULIAMONO,
        Shipped_font::ABEEZEE,
        Shipped_font::UBUNTU_MONO_BRONT,
        Shipped_font::JETBRAINS_MONO,
        Shipped_font::FIRA_CODE,
    };
}

}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    const QString mark = QString::fromLatin1(vnm_fonts::k_family_mark);

    // Property 1: the resource is reachable from a linking target once the
    // initialiser has been called, and the only path a consumer is told about
    // is the documented prefix.
    vnm_fonts::initialize_resources();
    QFile carried(QStringLiteral(":/vnm_fonts/RobotoCondensed-Regular.ttf"));
    check(carried.open(QIODevice::ReadOnly),
          QStringLiteral("the built-in font resource is not readable from a consumer target"));
    check(carried.size() > 0, QStringLiteral("the built-in font resource is empty"));

    QSet<QString> families;
    for (const vnm_fonts::Shipped_font font : every_shipped_font()) {
        const vnm_fonts::Registered_font registered = vnm_fonts::register_shipped_font(font);
        if (!check(registered.is_valid(),
                   QStringLiteral("shipped font %1: %2")
                       .arg(static_cast<int>(font)).arg(registered.error))) {
            continue;
        }
        check(registered.family.endsWith(mark),
              QStringLiteral("%1 does not carry the mark").arg(registered.family));
        check(QFontDatabase::families().contains(registered.family),
              QStringLiteral("%1 is not in the font database").arg(registered.family));

        // Property 3: the second registration is the first one.
        const vnm_fonts::Registered_font again = vnm_fonts::register_shipped_font(font);
        check(again.is_valid() && again.family == registered.family &&
                  again.font_id == registered.font_id,
              QStringLiteral("registering %1 twice produced %2 (id %3), not %4 (id %5)")
                  .arg(registered.family, again.family)
                  .arg(again.font_id)
                  .arg(registered.family)
                  .arg(registered.font_id));

        families.insert(registered.family);
    }

    // The two Roboto weights deliberately share one family, so twelve fonts make
    // eleven families. Anything fewer means two faces merged.
    check(families.size() == 11,
          QStringLiteral("the shipped fonts registered %1 families, expected 11: %2")
              .arg(families.size())
              .arg(QStringList(families.values()).join(QLatin1String(", "))));

    // Property 4: the pair the override exists for.
    check(families.contains(QStringLiteral("Font Awesome 7 Free (vnm)")) &&
              families.contains(QStringLiteral("Font Awesome 7 Free Solid (vnm)")),
          QStringLiteral("the two Font Awesome 7 Free faces did not register as two families"));

    if (s_failures > 0) {
        std::fprintf(stderr, "%d check(s) failed.\n", s_failures);
        return 1;
    }
    std::printf("PASS 12 shipped fonts register by id into 11 families.\n");
    return 0;
}
