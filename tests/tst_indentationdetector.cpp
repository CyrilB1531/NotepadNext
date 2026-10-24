/*
 * This file is part of Notepad Next.
 * Copyright 2026 Justin Dailey
 *
 * Notepad Next is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Notepad Next is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Notepad Next.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <QTest>

#include "IndentationDetector.h"


class TestIndentationDetector : public QObject
{
    Q_OBJECT

private slots:
    void detect_data();
    void detect();
};

void TestIndentationDetector::detect_data()
{
    QTest::addColumn<QByteArray>("text");
    QTest::addColumn<bool>("detected");
    QTest::addColumn<bool>("useTabs");
    QTest::addColumn<int>("indentSize");

    QTest::newRow("tabs")
        << QByteArray("def f():\n\tif a:\n\t\tb()\n\treturn 1\n") << true << true << 0;
    QTest::newRow("2 spaces")
        << QByteArray("f() {\n  if (a) {\n    b();\n  }\n  return 1;\n}\n") << true << false << 2;
    QTest::newRow("4 spaces")
        << QByteArray("def f():\n    if a:\n        b()\n    return 1\n") << true << false << 4;
    QTest::newRow("4 spaces with jumps of two levels")
        << QByteArray("a\n    b\n        c\n            d\na\n    b\n        c\n") << true << false << 4;
    QTest::newRow("CRLF")
        << QByteArray("a\r\n  b\r\n    c\r\n  d\r\n") << true << false << 2;
    QTest::newRow("CR")
        << QByteArray("a\r  b\r    c\r  d\r") << true << false << 2;
    QTest::newRow("blank lines do not break the indentation")
        << QByteArray("a\n  b\n\n   \n  c\n    d\n") << true << false << 2;
    QTest::newRow("block comment header is ignored")
        << QByteArray("/*\n * License\n * More\n * Even more\n */\n\nint f() {\n    a();\n    b();\n    c();\n}\n") << true << false << 4;
    QTest::newRow("block comment header does not outweigh tabs")
        << QByteArray("/*\n * License\n * More\n * Even more\n * And more\n */\n\nint f() {\n\ta();\n\tb();\n\tc();\n}\n") << true << true << 0;
    QTest::newRow("tab indented block comment counts as tabs")
        << QByteArray("f() {\n\t/*\n\t * comment\n\t */\n\ta();\n\tb();\n}\n") << true << true << 0;
    QTest::newRow("mostly tabs")
        << QByteArray("a\n\tb\n\tc\n\td\n  e\n") << true << true << 0;
    QTest::newRow("mostly spaces")
        << QByteArray("a\n  b\n  c\n  d\n\te\n") << true << false << 2;
    QTest::newRow("too few indented lines")
        << QByteArray("title\n  one line\n") << false << false << 0;
    QTest::newRow("as many tabs as spaces")
        << QByteArray("a\n\tb\n\tc\n  d\n  e\n") << false << false << 0;
    QTest::newRow("no indentation")
        << QByteArray("a\nb\nc\nd\n") << false << false << 0;
    QTest::newRow("empty")
        << QByteArray("") << false << false << 0;
    QTest::newRow("line starting with a star")
        << QByteArray("*a\n  b\n  c\n    d\n") << true << false << 2;
}

void TestIndentationDetector::detect()
{
    QFETCH(QByteArray, text);
    QFETCH(bool, detected);
    QFETCH(bool, useTabs);
    QFETCH(int, indentSize);

    const DetectedIndentation result = IndentationDetector::detect(text);

    QCOMPARE(result.detected, detected);
    QCOMPARE(result.useTabs, useTabs);
    QCOMPARE(result.indentSize, indentSize);
}

QTEST_MAIN(TestIndentationDetector)

#include "tst_indentationdetector.moc"
