#include <QtTest>

#include "../src/patterns.h"

class TestPatterns : public QObject
{
    Q_OBJECT

private slots:
    void toFilterEntry_data()
    {
        QTest::addColumn<QString>("pattern");
        QTest::addColumn<QString>("entry");

        QTest::newRow("FR prefix") << "33162######" << "^+33162";
        QTest::newRow("FR operator range") << "33151031###" << "^+33151031";
        QTest::newRow("FR exact") << "33148630000" << "+33148630000";
        QTest::newRow("short code") << "3244" << "3244";
        QTest::newRow("short code prefix") << "36170#" << "^36170";
        QTest::newRow("international prefix") << "3225####" << "^+3225";
        QTest::newRow("too short prefix") << "33#########" << "";
        QTest::newRow("letters") << "33abc######" << "";
        QTest::newRow("wildcard in the middle") << "331#2######" << "";
        QTest::newRow("plus sign") << "+33162######" << "";
        QTest::newRow("empty") << "" << "";
    }

    void toFilterEntry()
    {
        QFETCH(QString, pattern);
        QFETCH(QString, entry);
        QCOMPARE(::toFilterEntry(pattern), entry);
    }

    void mergeKeepsManualEntries()
    {
        const QStringList current = { "anonymous", "^+33162", "+33611111111", "^+33999" };
        const QStringList owned = { "^+33162", "^+33999" };
        const QStringList fresh = { "^+33162", "^+33163", "^+33163" };

        QCOMPARE(::merge(current, owned, fresh),
                 QStringList({ "anonymous", "+33611111111", "^+33162", "^+33163" }));
    }

    void mergeWithoutFreshRemovesOwned()
    {
        QCOMPARE(::merge({ "anonymous", "^+33162" }, { "^+33162" }, {}), QStringList({ "anonymous" }));
    }
};

QTEST_APPLESS_MAIN(TestPatterns)

#include "tst_patterns.moc"
