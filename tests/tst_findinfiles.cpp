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
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QTabBar>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>

#include "ApplicationSettings.h"
#include "EditorManager.h"
#include "FindReplaceDialog.h"
#include "MainWindow.h"
#include "NotepadNextApplication.h"
#include "ScintillaNext.h"
#include "SearchResultsDock.h"


static void writeFile(const QString &filePath, const QByteArray &content)
{
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(content);
}

class TestFindInFiles : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void menuOpensFindInFilesTab();
    void findInFilesWidgetsOnlyOnTheirTab();
    void searchesTextFilesAndSkipsBinaryFiles();
    void searchesUnsavedChangesOfOpenedFiles();
    void filtersAndSubFolders_data();
    void filtersAndSubFolders();
    void invalidDirectoryDoesNotSearch();
    void combinedSearchResults();
    void activatingResultOpensFile();
    void cancelStopsTheSearch();
    void settingsAreRemembered();

private:
    QTreeWidget *search(const QString &text, const QString &filters, bool subFolders, const QString &directory = QString());
    QStringList resultFileNames() const;
    QString resultSummary() const;
    bool isCurrentFile(const QString &filePath) const;

    FindReplaceDialog *dialog() const { return window->findChild<FindReplaceDialog *>(); }
    template<typename T> T *dialogWidget(const char *name) const { return dialog()->findChild<T *>(name); }
    QTreeWidget *resultsTree() const { return window->findChild<SearchResultsDock *>()->findChild<QTreeWidget *>(); }

    QTemporaryDir dir;
    MainWindow *window = nullptr;
};

void TestFindInFiles::initTestCase()
{
    QVERIFY(dir.isValid());

    writeFile(dir.filePath("a.txt"), "line one\nhello world\nhello again hello\n");
    writeFile(dir.filePath("c.txt"), "nothing here\n");
    writeFile(dir.filePath("bin.dat"), QByteArray("xx\0hello binary\n", 16));
    writeFile(dir.filePath("sub/b.cpp"), "int main() {\n  // hello from cpp\n}\n");

    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if ((window = qobject_cast<MainWindow *>(widget)))
            break;
    }
    QVERIFY(window);

    // The current file decides the default directory
    window->openFile(dir.filePath("a.txt"));
    QTRY_VERIFY(isCurrentFile(dir.filePath("a.txt")));
}

QTreeWidget *TestFindInFiles::search(const QString &text, const QString &filters, bool subFolders, const QString &directory)
{
    window->findChild<QAction *>("actionFindInFiles")->trigger();

    dialogWidget<QComboBox>("comboFind")->setCurrentText(text);
    dialogWidget<QComboBox>("comboFilters")->setCurrentText(filters);
    dialogWidget<QComboBox>("comboDirectory")->setCurrentText(directory.isEmpty() ? dir.path() : directory);
    dialogWidget<QCheckBox>("checkBoxInSubFolders")->setChecked(subFolders);
    dialogWidget<QPushButton>("buttonFindAllInFiles")->click();

    return resultsTree();
}

QStringList TestFindInFiles::resultFileNames() const
{
    QStringList names;
    const QTreeWidgetItem *search = resultsTree()->topLevelItem(0);

    for (int i = 0; i < search->childCount(); ++i) {
        // e.g. "/path/to/a.txt (3 hits)"
        names << QFileInfo(search->child(i)->text(0).section(QStringLiteral(" ("), 0, 0)).fileName();
    }

    names.sort();
    return names;
}

bool TestFindInFiles::isCurrentFile(const QString &filePath) const
{
    const ScintillaNext *editor = window->currentEditor();

    return editor && editor->isFile() && editor->getFileInfo() == QFileInfo(filePath);
}

QString TestFindInFiles::resultSummary() const
{
    return resultsTree()->topLevelItem(0)->text(0);
}

void TestFindInFiles::menuOpensFindInFilesTab()
{
    window->findChild<QAction *>("actionFindInFiles")->trigger();

    QVERIFY(dialog());
    QVERIFY(dialog()->isVisible());
    QCOMPARE(dialog()->findChild<QTabBar *>()->currentIndex(), static_cast<int>(FindReplaceDialog::FIND_IN_FILES_TAB));
    QCOMPARE(QDir(dialogWidget<QComboBox>("comboDirectory")->currentText()), QDir(dir.path()));
    QCOMPARE(dialogWidget<QComboBox>("comboFilters")->currentText(), QStringLiteral("*"));
    QVERIFY(dialogWidget<QCheckBox>("checkBoxInSubFolders")->isChecked());

    dialog()->close();
}

void TestFindInFiles::findInFilesWidgetsOnlyOnTheirTab()
{
    window->findChild<QAction *>("actionFindInFiles")->trigger();

    QVERIFY(dialogWidget<QPushButton>("buttonFindAllInFiles")->isVisible());
    QVERIFY(dialogWidget<QComboBox>("comboDirectory")->isVisible());
    QVERIFY(!dialogWidget<QPushButton>("buttonFind")->isVisible());

    // Enter triggers the search on this tab
    QVERIFY(dialogWidget<QPushButton>("buttonFindAllInFiles")->isDefault());

    for (int tab : {FindReplaceDialog::FIND_TAB, FindReplaceDialog::REPLACE_TAB, FindReplaceDialog::MARK_TAB}) {
        dialog()->setTab(tab);

        QVERIFY(!dialogWidget<QPushButton>("buttonFindAllInFiles")->isVisible());
        QVERIFY(!dialogWidget<QComboBox>("comboDirectory")->isVisible());
        QVERIFY(!dialogWidget<QComboBox>("comboFilters")->isVisible());
        QVERIFY(!dialogWidget<QCheckBox>("checkBoxInSubFolders")->isVisible());
        QVERIFY(!dialogWidget<QPushButton>("buttonFindAllInFiles")->isDefault());
    }

    dialog()->close();
}

void TestFindInFiles::searchesTextFilesAndSkipsBinaryFiles()
{
    search(QStringLiteral("hello"), QStringLiteral("*"), true);

    QCOMPARE(resultFileNames(), QStringList({"a.txt", "b.cpp"}));
    QCOMPARE(resultSummary(), QStringLiteral("Search \"hello\" (4 hits in 2 files)"));
    QVERIFY(!dialog()->isVisible());
}

void TestFindInFiles::searchesUnsavedChangesOfOpenedFiles()
{
    ScintillaNext *editor = window->currentEditor();
    const QByteArray original = editor->getText(editor->textLength() + 1);

    editor->appendText(15, "hello unsaved\n\n");

    search(QStringLiteral("unsaved"), QStringLiteral("*.txt"), true);

    QCOMPARE(resultFileNames(), QStringList({"a.txt"}));
    QCOMPARE(resultsTree()->topLevelItem(0)->child(0)->child(0)->text(1), QStringLiteral("hello unsaved"));

    editor->setText(original.constData());
    editor->setSavePoint();
}

void TestFindInFiles::filtersAndSubFolders_data()
{
    QTest::addColumn<QString>("filters");
    QTest::addColumn<bool>("subFolders");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("all, with sub-folders") << QStringLiteral("*") << true << QStringList({"a.txt", "b.cpp"});
    QTest::newRow("all, without sub-folders") << QStringLiteral("*") << false << QStringList({"a.txt"});
    QTest::newRow("*.cpp") << QStringLiteral("*.cpp") << true << QStringList({"b.cpp"});
    QTest::newRow("separated by ;") << QStringLiteral("*.txt;*.cpp") << true << QStringList({"a.txt", "b.cpp"});
    QTest::newRow("separated by spaces") << QStringLiteral("*.txt *.cpp") << true << QStringList({"a.txt", "b.cpp"});
    QTest::newRow("empty means all") << QString() << true << QStringList({"a.txt", "b.cpp"});
}

void TestFindInFiles::filtersAndSubFolders()
{
    QFETCH(QString, filters);
    QFETCH(bool, subFolders);
    QFETCH(QStringList, expected);

    search(QStringLiteral("hello"), filters, subFolders);

    QCOMPARE(resultFileNames(), expected);
}

void TestFindInFiles::invalidDirectoryDoesNotSearch()
{
    resultsTree()->clear();

    search(QStringLiteral("hello"), QStringLiteral("*"), true, dir.filePath("does-not-exist"));

    QCOMPARE(resultsTree()->topLevelItemCount(), 0);
    QVERIFY(dialog()->isVisible());
    QVERIFY(!dialog()->findChild<QStatusBar *>()->currentMessage().isEmpty());

    dialog()->close();
}

void TestFindInFiles::combinedSearchResults()
{
    ApplicationSettings *settings = qobject_cast<NotepadNextApplication *>(qApp)->getSettings();
    const bool combine = settings->combineSearchResults();

    // "hello again hello" is listed twice unless the results are combined
    settings->setCombineSearchResults(false);
    search(QStringLiteral("hello"), QStringLiteral("a.txt"), true);
    QCOMPARE(resultsTree()->topLevelItem(0)->child(0)->childCount(), 3);

    settings->setCombineSearchResults(true);
    search(QStringLiteral("hello"), QStringLiteral("a.txt"), true);
    QCOMPARE(resultsTree()->topLevelItem(0)->child(0)->childCount(), 2);
    QVERIFY(resultsTree()->topLevelItem(0)->child(0)->text(0).endsWith(QStringLiteral("(3 hits)")));
    QCOMPARE(resultSummary(), QStringLiteral("Search \"hello\" (3 hits in 1 files)"));

    settings->setCombineSearchResults(combine);
}

void TestFindInFiles::activatingResultOpensFile()
{
    const QString filePath = dir.filePath("sub/b.cpp");
    QVERIFY(!qobject_cast<NotepadNextApplication *>(qApp)->getEditorManager()->getEditorByFilePath(filePath));

    QTreeWidget *tree = search(QStringLiteral("hello"), QStringLiteral("*.cpp"), true);
    emit tree->itemActivated(tree->topLevelItem(0)->child(0)->child(0), 0);

    QTRY_VERIFY(isCurrentFile(filePath));

    ScintillaNext *editor = window->currentEditor();
    QCOMPARE(editor->lineFromPosition(editor->selectionStart()), 1);
    QCOMPARE(QString::fromUtf8(editor->get_text_range(editor->selectionStart(), editor->selectionEnd())), QStringLiteral("hello"));

    window->openFile(dir.filePath("a.txt"));
    QTRY_VERIFY(isCurrentFile(dir.filePath("a.txt")));
}

void TestFindInFiles::cancelStopsTheSearch()
{
    QTemporaryDir manyFiles;
    QVERIFY(manyFiles.isValid());

    const int fileCount = 3000;
    for (int i = 0; i < fileCount; ++i) {
        writeFile(manyFiles.filePath(QStringLiteral("f%1.txt").arg(i)), "hello\n");
    }

    // Cancel as soon as the progress dialog shows up
    bool progressShown = false;
    QTimer canceller;
    connect(&canceller, &QTimer::timeout, this, [&]() {
        for (QProgressDialog *progress : dialog()->findChildren<QProgressDialog *>()) {
            if (progress->isVisible() && !progressShown) {
                progressShown = true;
                progress->cancel();
            }
        }
    });
    canceller.start(10);

    ScintillaNext *editor = window->currentEditor();
    search(QStringLiteral("hello"), QStringLiteral("*"), true, manyFiles.path());
    canceller.stop();

    QVERIFY(progressShown);

    const int searchedFiles = resultsTree()->topLevelItem(0)->childCount();
    QVERIFY(searchedFiles > 0);
    QVERIFY(searchedFiles < fileCount);

    QCOMPARE(window->currentEditor(), editor);
    QVERIFY(!dialog()->isVisible());

    // The next search is not affected
    search(QStringLiteral("hello"), QStringLiteral("*.cpp"), true);
    QCOMPARE(resultFileNames(), QStringList({"b.cpp"}));
}

void TestFindInFiles::settingsAreRemembered()
{
    search(QStringLiteral("hello"), QStringLiteral("*.cpp"), false);

    ApplicationSettings settings;
    settings.beginGroup("FindReplaceDialog");

    QCOMPARE(settings.value("RecentDirectoryList").toStringList().value(0), QDir::cleanPath(dir.path()));
    QCOMPARE(settings.value("RecentFiltersList").toStringList().value(0), QStringLiteral("*.cpp"));
    QCOMPARE(settings.value("InSubFolders").toBool(), false);
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

    TestFindInFiles test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_findinfiles.moc"
