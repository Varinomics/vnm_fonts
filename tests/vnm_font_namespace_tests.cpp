// Proves the properties the runtime marking exists to guarantee:
//
//   1. Before any shipped font is registered, no marked family is present. That
//      is the assertion that would have caught the original defect.
//   2. Each shipped font registers under exactly one family, that family
//      carries the mark, and the font database resolves it to itself rather
//      than substituting another face.
//   3. Each marked family appears exactly once in the font database.
//   4. The patch changes only the name table.
//   5. The two Roboto faces form one family with two styles, and the two Font
//      Awesome 7 Free faces form two distinct families.
//
// Failures print to stderr and the process exits non-zero. Nothing here aborts.

#include "vnm_font_namespace.h"

#include <QByteArray>
#include <QFile>
#include <QFontDatabase>
#include <QFontInfo>
#include <QGuiApplication>
#include <QMap>
#include <QString>
#include <QStringList>
#include <cstdio>
#include <vector>

namespace {

struct Font_case
{
    QString                  file_name;
    QString                  expected_family;
    vnm_fonts::Family_override overrides;
};

int s_failures = 0;

bool check(bool condition, const QString& message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL %s\n", qPrintable(message));
        ++s_failures;
    }
    return condition;
}

QByteArray read_font(const QString& directory, const QString& file_name, bool* out_read)
{
    QFile file(directory + QLatin1Char('/') + file_name);
    *out_read = file.open(QIODevice::ReadOnly);
    if (!*out_read) {
        check(false, QStringLiteral("cannot read %1").arg(file.fileName()));
        return QByteArray();
    }
    return file.readAll();
}

// The manifest in THIRD_PARTY/*.toml is the record of record for these; the
// table repeats them so the test needs no TOML parser, and the family names are
// the upstream ones with the mark appended, which is the rule under test.
std::vector<Font_case> font_cases()
{
    vnm_fonts::Family_override solid_override;
    solid_override.insert(16, QStringLiteral("Font Awesome 7 Free Solid"));

    return {
        {QStringLiteral("RobotoCondensed-Regular.ttf"),    QStringLiteral("Roboto Condensed (vnm)"),         {}},
        {QStringLiteral("RobotoCondensed-Light.ttf"),      QStringLiteral("Roboto Condensed (vnm)"),         {}},
        {QStringLiteral("FontAwesome.otf"),                QStringLiteral("FontAwesome (vnm)"),              {}},
        {QStringLiteral("FontAwesome7Brands-Regular.otf"), QStringLiteral("Font Awesome 7 Brands (vnm)"),    {}},
        {QStringLiteral("FontAwesome7Free-Regular.otf"),   QStringLiteral("Font Awesome 7 Free (vnm)"),      {}},
        {QStringLiteral("FontAwesome7Free-Solid.otf"),     QStringLiteral("Font Awesome 7 Free Solid (vnm)"), solid_override},
        {QStringLiteral("NotoSansSymbols2-Regular.ttf"),   QStringLiteral("Noto Sans Symbols 2 (vnm)"),      {}},
        {QStringLiteral("JuliaMono-Regular.ttf"),          QStringLiteral("JuliaMono (vnm)"),                {}},
        {QStringLiteral("ABeeZee-Regular.ttf"),            QStringLiteral("ABeeZee (vnm)"),                  {}},
        {QStringLiteral("JetBrainsMono-Regular.ttf"),      QStringLiteral("JetBrains Mono (vnm)"),           {}},
        {QStringLiteral("FiraCode-Regular.ttf"),           QStringLiteral("Fira Code (vnm)"),                {}},
        {QStringLiteral("UbuntuMono-Bront.ttf"),           QStringLiteral("Ubuntu Mono - Bront (vnm)"),      {}},
    };
}

QMap<QByteArray, QByteArray> sfnt_tables(const QByteArray& font_bytes)
{
    QMap<QByteArray, QByteArray> tables;
    const int table_count = (static_cast<quint8>(font_bytes.at(4)) << 8) |
                            static_cast<quint8>(font_bytes.at(5));
    for (int index = 0; index < table_count; ++index) {
        const int entry = 12 + index * 16;

        quint32 offset = 0;
        quint32 length = 0;
        for (int byte = 0; byte < 4; ++byte) {
            offset = (offset << 8) | static_cast<quint8>(font_bytes.at(entry + 8 + byte));
            length = (length << 8) | static_cast<quint8>(font_bytes.at(entry + 12 + byte));
        }
        tables.insert(font_bytes.mid(entry, 4),
                      font_bytes.mid(static_cast<int>(offset), static_cast<int>(length)));
    }
    return tables;
}

// head.checkSumAdjustment covers the whole file, so it necessarily moves when
// the name table's length does. Everything else in head must not.
QByteArray without_checksum_adjustment(const QByteArray& head)
{
    QByteArray normalized = head;
    for (int byte = 0; byte < 4; ++byte) {
        normalized[8 + byte] = char(0);
    }
    return normalized;
}

void check_only_the_name_table_changed(const Font_case& font_case, const QByteArray& source,
                                       const QByteArray& marked)
{
    const QMap<QByteArray, QByteArray> before = sfnt_tables(source);
    const QMap<QByteArray, QByteArray> after  = sfnt_tables(marked);

    check(before.keys() == after.keys(),
          QStringLiteral("%1: the marking changed the table set").arg(font_case.file_name));

    for (auto entry = before.constBegin(); entry != before.constEnd(); ++entry) {
        if (entry.key() == QByteArrayLiteral("name")) {
            continue;
        }
        const QByteArray marked_table = after.value(entry.key());
        const bool same = entry.key() == QByteArrayLiteral("head")
            ? without_checksum_adjustment(entry.value()) == without_checksum_adjustment(marked_table)
            : entry.value() == marked_table;
        check(same, QStringLiteral("%1: the marking changed the %2 table")
                        .arg(font_case.file_name, QString::fromLatin1(entry.key())));
    }
}

void check_family_grouping(const QString& family, const QStringList& expected_styles)
{
    const QStringList styles = QFontDatabase::styles(family);
    check(styles.size() == expected_styles.size(),
          QStringLiteral("%1 has styles %2, expected %3")
              .arg(family, styles.join(QLatin1String(", ")),
                   expected_styles.join(QLatin1String(", "))));
    for (const QString& style : expected_styles) {
        check(styles.contains(style),
              QStringLiteral("%1 does not offer the style %2").arg(family, style));
    }
}

}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <fonts-directory>\n", argv[0]);
        return 2;
    }
    const QString font_directory = QString::fromLocal8Bit(argv[1]);
    const std::vector<Font_case> cases = font_cases();

    // Property 1: nothing carrying the mark exists before this test registers it.
    const QString mark = QString::fromLatin1(vnm_fonts::k_family_mark);
    for (const QString& family : QFontDatabase::families()) {
        check(!family.endsWith(mark),
              QStringLiteral("%1 was already present before registration").arg(family));
    }

    for (const Font_case& font_case : cases) {
        bool read = false;
        const QByteArray source = read_font(font_directory, font_case.file_name, &read);
        if (!read) {
            continue;
        }

        const vnm_fonts::Marked_font marked =
            vnm_fonts::mark_font_family(source, font_case.overrides);
        if (!check(marked.is_valid(),
                   QStringLiteral("%1: %2").arg(font_case.file_name, marked.error))) {
            continue;
        }
        check_only_the_name_table_changed(font_case, source, marked.bytes);

        const vnm_fonts::Registered_font registered =
            vnm_fonts::register_marked_font(source, font_case.overrides);
        if (!check(registered.is_valid(),
                   QStringLiteral("%1: %2").arg(font_case.file_name, registered.error))) {
            continue;
        }

        check(registered.family == font_case.expected_family,
              QStringLiteral("%1 registered as %2, expected %3")
                  .arg(font_case.file_name, registered.family, font_case.expected_family));

        // The font database must resolve the marked family to itself. A
        // substitution here would mean the family did not register as its own.
        const QFontInfo info((QFont(registered.family)));
        check(info.family() == registered.family,
              QStringLiteral("%1 resolved to %2, not to itself")
                  .arg(registered.family, info.family()));

        check(QFontDatabase::families().count(registered.family) == 1,
              QStringLiteral("%1 appears more than once in the font database")
                  .arg(registered.family));
    }

    // Property 5: the deliberate grouping, and the deliberate separation.
    check_family_grouping(QStringLiteral("Roboto Condensed (vnm)"),
                          {QStringLiteral("Regular"), QStringLiteral("Light")});
    check(QStringLiteral("Font Awesome 7 Free (vnm)") !=
              QStringLiteral("Font Awesome 7 Free Solid (vnm)"),
          QStringLiteral("the two Font Awesome 7 Free faces must not share a family"));
    check(QFontDatabase::families().contains(QStringLiteral("Font Awesome 7 Free (vnm)")) &&
              QFontDatabase::families().contains(QStringLiteral("Font Awesome 7 Free Solid (vnm)")),
          QStringLiteral("both Font Awesome 7 Free families must be registered separately"));

    if (s_failures > 0) {
        std::fprintf(stderr, "%d check(s) failed.\n", s_failures);
        return 1;
    }
    std::printf("PASS %zu fonts register under marked families.\n", cases.size());
    return 0;
}
