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


#include <QAction>
#include <QComboBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSettings>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>

#include "FindReplaceDialog.h"
#include "MainWindow.h"
#include "NotepadNextApplication.h"
#include "ScintillaNext.h"
#include "SearchResultsDock.h"


// Two tabs and the literal text \t
static const QByteArray Content("col1\tcol2\tcol3\nliteral \\t here\nend\n");

class TestExtendedSearch : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void findNextAndCount_data();
    void findNextAndCount();
    void findAllAndMarkAll();
    void findNextAfterClosingTheDialog();
    void replaceAll_data();
    void replaceAll();
    void nullCharacter();
    void normalMode();
    void regexMode();
    void searchResultsUnchanged_data();
    void searchResultsUnchanged();

private:
    void setMode(const char *radio);
    void click(const char *button);
    void setText(const char *combo, const QString &text);
    int messageCount() const;
    QString selectedText() const;
    QByteArray documentText() const;
    QString lastLine() const;

    FindReplaceDialog *dialog() const { return window->findChild<FindReplaceDialog *>(); }
    QTreeWidget *resultsTree() const { return window->findChild<SearchResultsDock *>()->findChild<QTreeWidget *>(); }

    MainWindow *window = nullptr;
    ScintillaNext *editor = nullptr;
};

void TestExtendedSearch::initTestCase()
{
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if ((window = qobject_cast<MainWindow *>(widget)))
            break;
    }
    QVERIFY(window);

    QTRY_VERIFY(window->currentEditor());
    editor = window->currentEditor();
}

void TestExtendedSearch::init()
{
    editor->setText(Content.constData());

    window->findChild<QAction *>("actionFind")->trigger();
    QVERIFY(dialog()->isVisible());

    // Opening the dialog selects the word at the caret, so start from the beginning afterwards
    editor->gotoPos(0);
    dialog()->setTab(FindReplaceDialog::FIND_TAB);
    setMode("radioExtendedSearch");
}

void TestExtendedSearch::setMode(const char *radio)
{
    dialog()->findChild<QRadioButton *>(radio)->setChecked(true);
}

void TestExtendedSearch::click(const char *button)
{
    dialog()->findChild<QPushButton *>(button)->click();
}

void TestExtendedSearch::setText(const char *combo, const QString &text)
{
    dialog()->findChild<QComboBox *>(combo)->setCurrentText(text);
}

int TestExtendedSearch::messageCount() const
{
    // e.g. "Found 2 matches" or "Mark: 2 matches in entire file"
    const QRegularExpressionMatch match = QRegularExpression(QStringLiteral("\\d+")).match(dialog()->findChild<QStatusBar *>()->currentMessage());

    return match.hasMatch() ? match.captured().toInt() : -1;
}

QString TestExtendedSearch::selectedText() const
{
    return QString::fromUtf8(editor->get_text_range(editor->selectionStart(), editor->selectionEnd()));
}

QByteArray TestExtendedSearch::documentText() const
{
    return editor->getText(editor->textLength() + 1).left(editor->textLength());
}

QString TestExtendedSearch::lastLine() const
{
    const QString text = QString::fromUtf8(documentText());

    return text.section('\n', -2, -2);
}

void TestExtendedSearch::findNextAndCount_data()
{
    QTest::addColumn<QString>("search");
    QTest::addColumn<QString>("selected");
    QTest::addColumn<int>("count");

    QTest::newRow("\\t") << QStringLiteral("\\t") << QStringLiteral("\t") << 2;
    QTest::newRow("\\n across lines") << QStringLiteral("col3\\nliteral") << QStringLiteral("col3\nliteral") << 1;
    QTest::newRow("escaped backslash") << QStringLiteral("\\\\t") << QStringLiteral("\\t") << 1;
    QTest::newRow("\\x") << QStringLiteral("\\x09") << QStringLiteral("\t") << 2;
    QTest::newRow("\\u") << QStringLiteral("\\u0009") << QStringLiteral("\t") << 2;
    QTest::newRow("\\o") << QStringLiteral("\\o011") << QStringLiteral("\t") << 2;
    QTest::newRow("\\d") << QStringLiteral("\\d009") << QStringLiteral("\t") << 2;
    QTest::newRow("\\b") << QStringLiteral("\\b00001001") << QStringLiteral("\t") << 2;
    QTest::newRow("upper case hex in a word") << QStringLiteral("c\\x6Fl3") << QStringLiteral("col3") << 1;
    QTest::newRow("lower case hex in a word") << QStringLiteral("c\\x6fl3") << QStringLiteral("col3") << 1;
}

void TestExtendedSearch::findNextAndCount()
{
    QFETCH(QString, search);
    QFETCH(QString, selected);
    QFETCH(int, count);

    setText("comboFind", search);

    click("buttonFind");
    QCOMPARE(selectedText(), selected);

    click("buttonCount");
    QCOMPARE(messageCount(), count);
}

void TestExtendedSearch::findAllAndMarkAll()
{
    setText("comboFind", QStringLiteral("\\t"));

    click("buttonFindAllInCurrent");
    QCOMPARE(resultsTree()->topLevelItem(0)->text(0), QStringLiteral("Search \"\\t\" (2 hits in 1 files)"));

    init();
    setText("comboFind", QStringLiteral("\\t"));
    click("buttonFindAllInDocuments");
    QCOMPARE(resultsTree()->topLevelItem(0)->text(0), QStringLiteral("Search \"\\t\" (2 hits in 1 files)"));

    init();
    dialog()->setTab(FindReplaceDialog::MARK_TAB);
    setText("comboFind", QStringLiteral("\\t"));
    click("buttonMarkAll");
    QCOMPARE(messageCount(), 2);
}

void TestExtendedSearch::findNextAfterClosingTheDialog()
{
    setText("comboFind", QStringLiteral("\\t"));
    click("buttonFind");
    dialog()->close();

    editor->gotoPos(0);
    window->findChild<QAction *>("actionFindNext")->trigger();

    QCOMPARE(selectedText(), QStringLiteral("\t"));
}

void TestExtendedSearch::replaceAll_data()
{
    QTest::addColumn<QString>("replacement");
    QTest::addColumn<QString>("expected");

    QTest::newRow("\\t") << QStringLiteral("a\\tb") << QStringLiteral("a\tb");
    QTest::newRow("escaped backslash") << QStringLiteral("\\\\n") << QStringLiteral("\\n");
    QTest::newRow("\\u and \\x") << QStringLiteral("\\u00E9t\\xE9") << QStringLiteral("été");
    QTest::newRow("consecutive codes") << QStringLiteral("\\x41\\x42") << QStringLiteral("AB");
    QTest::newRow("\\d, \\o and \\b") << QStringLiteral("\\d065\\o102\\b01000011") << QStringLiteral("ABC");
    QTest::newRow("escaped code") << QStringLiteral("\\\\x41") << QStringLiteral("\\x41");
    QTest::newRow("unknown") << QStringLiteral("\\q") << QStringLiteral("\\q");
    QTest::newRow("trailing backslash") << QStringLiteral("x\\") << QStringLiteral("x\\");
    QTest::newRow("incomplete") << QStringLiteral("\\x6") << QStringLiteral("\\x6");
    QTest::newRow("invalid digit") << QStringLiteral("\\xZZ") << QStringLiteral("\\xZZ");
    QTest::newRow("0x prefix") << QStringLiteral("\\x0x1") << QStringLiteral("\\x0x1");
    QTest::newRow("sign") << QStringLiteral("\\x+1") << QStringLiteral("\\x+1");
    QTest::newRow("decimal out of range") << QStringLiteral("\\d999") << QStringLiteral("\\d999");
    QTest::newRow("octal out of range") << QStringLiteral("\\o400") << QStringLiteral("\\o400");
    QTest::newRow("incomplete binary") << QStringLiteral("\\b0100000") << QStringLiteral("\\b0100000");
    QTest::newRow("invalid binary digit") << QStringLiteral("\\b01000012") << QStringLiteral("\\b01000012");
}

void TestExtendedSearch::replaceAll()
{
    QFETCH(QString, replacement);
    QFETCH(QString, expected);

    dialog()->setTab(FindReplaceDialog::REPLACE_TAB);
    setText("comboFind", QStringLiteral("end"));
    setText("comboReplace", replacement);
    click("buttonReplaceAll");

    QCOMPARE(lastLine(), expected);
}

void TestExtendedSearch::nullCharacter()
{
    // \0 in the replacement inserts a null character
    dialog()->setTab(FindReplaceDialog::REPLACE_TAB);
    setText("comboFind", QStringLiteral("\\t"));
    setText("comboReplace", QStringLiteral("\\0"));
    click("buttonReplaceAll");
    QCOMPARE(documentText(), QByteArray("col1\0col2\0col3\nliteral \\t here\nend\n", 35));

    // \0 finds the null characters with every action
    setText("comboFind", QStringLiteral("\\0"));
    editor->gotoPos(0);
    click("buttonFind");
    QCOMPARE(editor->selectionStart(), 4);
    QCOMPARE(editor->selectionEnd(), 5);

    click("buttonCount");
    QCOMPARE(messageCount(), 2);

    click("buttonFindAllInCurrent");
    QCOMPARE(resultsTree()->topLevelItem(0)->text(0), QStringLiteral("Search \"\\0\" (2 hits in 1 files)"));

    window->findChild<QAction *>("actionFind")->trigger();
    dialog()->setTab(FindReplaceDialog::MARK_TAB);
    setText("comboFind", QStringLiteral("\\0"));
    click("buttonMarkAll");
    QCOMPARE(messageCount(), 2);

    dialog()->setTab(FindReplaceDialog::REPLACE_TAB);
    setText("comboReplace", QStringLiteral("|"));
    click("buttonReplaceAll");
    QCOMPARE(documentText(), QByteArray("col1|col2|col3\nliteral \\t here\nend\n"));
}

void TestExtendedSearch::normalMode()
{
    setMode("radioNormalSearch");

    // \t is searched literally
    setText("comboFind", QStringLiteral("\\t"));
    click("buttonFind");
    QCOMPARE(selectedText(), QStringLiteral("\\t"));

    click("buttonCount");
    QCOMPARE(messageCount(), 1);

    click("buttonFindAllInCurrent");
    QCOMPARE(resultsTree()->topLevelItem(0)->text(0), QStringLiteral("Search \"\\t\" (1 hits in 1 files)"));
    QCOMPARE(resultsTree()->topLevelItem(0)->child(0)->child(0)->text(1), QStringLiteral("literal \\t here"));

    // A real tab character is still found
    init();
    setMode("radioNormalSearch");
    setText("comboFind", QStringLiteral("\t"));
    click("buttonCount");
    QCOMPARE(messageCount(), 2);

    // Replace All only replaces the literal \t
    dialog()->setTab(FindReplaceDialog::REPLACE_TAB);
    setText("comboFind", QStringLiteral("\\t"));
    setText("comboReplace", QStringLiteral("X"));
    click("buttonReplaceAll");
    QCOMPARE(documentText(), QByteArray("col1\tcol2\tcol3\nliteral X here\nend\n"));
}

void TestExtendedSearch::regexMode()
{
    // The regular expression engine handles \t itself
    setMode("radioRegexSearch");
    setText("comboFind", QStringLiteral("col\\d\\tcol"));
    click("buttonFind");

    QCOMPARE(selectedText(), QStringLiteral("col1\tcol"));
}

void TestExtendedSearch::searchResultsUnchanged_data()
{
    QTest::addColumn<QString>("mode");
    QTest::addColumn<QString>("search");
    QTest::addColumn<QString>("replacement");
    QTest::addColumn<QByteArray>("expected");
    QTest::addColumn<int>("count");

    // Matches are now searched in the target, the results must stay the same
    QTest::newRow("regex groups") << QStringLiteral("radioRegexSearch") << QStringLiteral("(col)(\\d)") << QStringLiteral("\\2\\1") << QByteArray("1col\t2col\t3col\nliteral \\t here\nend\n") << 3;
    QTest::newRow("regex \\w+") << QStringLiteral("radioRegexSearch") << QStringLiteral("l\\w+") << QStringLiteral("W") << QByteArray("coW\tcoW\tcoW\nW \\t here\nend\n") << 4;
    QTest::newRow("normal") << QStringLiteral("radioNormalSearch") << QStringLiteral("col") << QStringLiteral("COL") << QByteArray("COL1\tCOL2\tCOL3\nliteral \\t here\nend\n") << 3;
    QTest::newRow("longer replacement") << QStringLiteral("radioNormalSearch") << QStringLiteral("e") << QStringLiteral("ee") << QByteArray("col1\tcol2\tcol3\nliteeral \\t heeree\neend\n") << 4;
    QTest::newRow("case insensitive") << QStringLiteral("radioNormalSearch") << QStringLiteral("COL") << QStringLiteral("x") << QByteArray("x1\tx2\tx3\nliteral \\t here\nend\n") << 3;
}

void TestExtendedSearch::searchResultsUnchanged()
{
    QFETCH(QString, mode);
    QFETCH(QString, search);
    QFETCH(QString, replacement);
    QFETCH(QByteArray, expected);
    QFETCH(int, count);

    setMode(qPrintable(mode));
    setText("comboFind", search);

    click("buttonCount");
    QCOMPARE(messageCount(), count);

    dialog()->setTab(FindReplaceDialog::REPLACE_TAB);
    setText("comboReplace", replacement);
    click("buttonReplaceAll");
    QCOMPARE(documentText(), expected);
    QCOMPARE(messageCount(), count);
}

int main(int argc, char *argv[])
{
    // Use a separate name so a running Notepad Next is never contacted, and keep the settings
    // in a temporary directory so the user's settings are left untouched
    QApplication::setOrganizationName("NotepadNext");
    QApplication::setApplicationName("NotepadNextTests");
    QApplication::setApplicationVersion(APP_VERSION);
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QTemporaryDir settingsDir;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());

    // The application parses its own command line, so do not pass the test arguments to it
    static int appArgc = 1;
    NotepadNextApplication app(appArgc, argv);
    app.init();

    TestExtendedSearch test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_extendedsearch.moc"
