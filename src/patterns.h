#ifndef PATTERNS_H
#define PATTERNS_H

#include <QRegularExpression>
#include <QSet>
#include <QStringList>

// Converts a Saracroche pattern (digits with trailing '#' wildcards, e.g. "33162######")
// into a voicecall filter entry ("+33162…" exact, "^+33162" prefix). Empty when invalid.
inline QString toFilterEntry(const QString &pattern)
{
    static const QRegularExpression valid(QStringLiteral("^[0-9]+#*$"));
    if (!valid.match(pattern).hasMatch())
        return QString();

    QString literal = pattern;
    while (literal.endsWith(QLatin1Char('#')))
        literal.chop(1);
    const bool wildcard = literal.length() < pattern.length();
    // A too short prefix from a corrupted list would block whole countries.
    if (wildcard && literal.length() < 4)
        return QString();

    // shortcut: ^ drops Saracroche's fixed-length check, fine for fixed-length FR numbers
    // shortcut: only the +33 form, add ^0 variants if a carrier delivers national caller IDs
    const QString number = pattern.length() <= 6 ? literal : QLatin1Char('+') + literal;
    return wildcard ? QLatin1Char('^') + number : number;
}

// Keeps the entries the user added by hand (anything not written by us last time),
// then appends the fresh entries, without duplicates.
inline QStringList merge(const QStringList &current, const QStringList &owned, const QStringList &fresh)
{
    QSet<QString> ownedSet;
    for (const QString &entry : owned)
        ownedSet.insert(entry);

    QStringList result;
    QSet<QString> seen;
    for (const QString &entry : current) {
        if (!ownedSet.contains(entry) && !seen.contains(entry)) {
            seen.insert(entry);
            result.append(entry);
        }
    }
    for (const QString &entry : fresh) {
        if (!seen.contains(entry)) {
            seen.insert(entry);
            result.append(entry);
        }
    }
    return result;
}

#endif
