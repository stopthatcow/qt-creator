import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "TextEditor"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "network", "script", "printsupport"] }
    Depends { name: "Core" }
    Depends { name: "Find" }
    Depends { name: "Locator" }

    cpp.includePaths: base.concat([
        "generichighlighter",
        "snippets",
        "codeassist",
        "."
    ])

    files: [
        "TextEditor.mimetypes.xml",
        "autocompleter.cpp",
        "autocompleter.h",
        "basefilefind.cpp",
        "basefilefind.h",
        "basefilefind_p.h",
        "basehoverhandler.cpp",
        "basehoverhandler.h",
        "basetextdocument.cpp",
        "basetextdocument.h",
        "basetextdocumentlayout.cpp",
        "basetextdocumentlayout.h",
        "basetexteditor.cpp",
        "basetexteditor.h",
        "basetexteditor_p.h",
        "basetextmark.cpp",
        "basetextmark.h",
        "basetextmarkregistry.h",
        "behaviorsettings.cpp",
        "behaviorsettings.h",
        "behaviorsettingspage.cpp",
        "behaviorsettingspage.h",
        "behaviorsettingspage.ui",
        "behaviorsettingswidget.cpp",
        "behaviorsettingswidget.h",
        "behaviorsettingswidget.ui",
        "circularclipboard.cpp",
        "circularclipboard.h",
        "circularclipboardassist.cpp",
        "circularclipboardassist.h",
        "codecselector.cpp",
        "codecselector.h",
        "codestyleeditor.cpp",
        "codestyleeditor.h",
        "codestylepool.cpp",
        "codestylepool.h",
        "codestyleselectorwidget.cpp",
        "codestyleselectorwidget.h",
        "codestyleselectorwidget.ui",
        "colorscheme.cpp",
        "colorscheme.h",
        "colorschemeedit.cpp",
        "colorschemeedit.h",
        "colorschemeedit.ui",
        "completionsettings.cpp",
        "completionsettings.h",
        "convenience.cpp",
        "convenience.h",
        "displaysettings.cpp",
        "displaysettings.h",
        "displaysettingspage.cpp",
        "displaysettingspage.h",
        "displaysettingspage.ui",
        "extraencodingsettings.cpp",
        "extraencodingsettings.h",
        "findincurrentfile.cpp",
        "findincurrentfile.h",
        "findinfiles.cpp",
        "findinfiles.h",
        "findinopenfiles.cpp",
        "findinopenfiles.h",
        "fontsettings.cpp",
        "fontsettings.h",
        "fontsettingspage.cpp",
        "fontsettingspage.h",
        "fontsettingspage.ui",
        "helpitem.cpp",
        "helpitem.h",
        "icodestylepreferences.cpp",
        "icodestylepreferences.h",
        "icodestylepreferencesfactory.cpp",
        "icodestylepreferencesfactory.h",
        "indenter.cpp",
        "indenter.h",
        "ioutlinewidget.h",
        "itexteditor.cpp",
        "itexteditor.h",
        "itextmark.cpp",
        "itextmark.h",
        "linenumberfilter.cpp",
        "linenumberfilter.h",
        "normalindenter.cpp",
        "normalindenter.h",
        "outlinefactory.cpp",
        "outlinefactory.h",
        "plaintexteditor.cpp",
        "plaintexteditor.h",
        "plaintexteditorfactory.cpp",
        "plaintexteditorfactory.h",
        "quickfix.cpp",
        "quickfix.h",
        "refactoringchanges.cpp",
        "refactoringchanges.h",
        "refactoroverlay.cpp",
        "refactoroverlay.h",
        "semantichighlighter.cpp",
        "semantichighlighter.h",
        "simplecodestylepreferences.cpp",
        "simplecodestylepreferences.h",
        "simplecodestylepreferenceswidget.cpp",
        "simplecodestylepreferenceswidget.h",
        "storagesettings.cpp",
        "storagesettings.h",
        "syntaxhighlighter.cpp",
        "syntaxhighlighter.h",
        "tabsettings.cpp",
        "tabsettings.h",
        "tabsettingswidget.cpp",
        "tabsettingswidget.h",
        "tabsettingswidget.ui",
        "texteditor.qrc",
        "texteditor_global.h",
        "texteditoractionhandler.cpp",
        "texteditoractionhandler.h",
        "texteditorconstants.cpp",
        "texteditorconstants.h",
        "texteditoroptionspage.cpp",
        "texteditoroptionspage.h",
        "texteditoroverlay.cpp",
        "texteditoroverlay.h",
        "texteditorplugin.cpp",
        "texteditorplugin.h",
        "texteditorsettings.cpp",
        "texteditorsettings.h",
        "textfilewizard.cpp",
        "textfilewizard.h",
        "typingsettings.cpp",
        "typingsettings.h",
    ]

    Group {
        prefix: "codeassist/"
        files: [
            "assistenums.h",
            "basicproposalitem.cpp",
            "basicproposalitem.h",
            "basicproposalitemlistmodel.cpp",
            "basicproposalitemlistmodel.h",
            "codeassistant.cpp",
            "codeassistant.h",
            "completionassistprovider.cpp",
            "completionassistprovider.h",
            "defaultassistinterface.cpp",
            "defaultassistinterface.h",
            "functionhintproposal.cpp",
            "functionhintproposal.h",
            "functionhintproposalwidget.cpp",
            "functionhintproposalwidget.h",
            "genericproposal.cpp",
            "genericproposal.h",
            "genericproposalwidget.cpp",
            "genericproposalwidget.h",
            "iassistinterface.cpp",
            "iassistinterface.h",
            "iassistprocessor.cpp",
            "iassistprocessor.h",
            "iassistproposal.cpp",
            "iassistproposal.h",
            "iassistproposalitem.cpp",
            "iassistproposalitem.h",
            "iassistproposalmodel.cpp",
            "iassistproposalmodel.h",
            "iassistproposalwidget.cpp",
            "iassistproposalwidget.h",
            "iassistprovider.cpp",
            "iassistprovider.h",
            "ifunctionhintproposalmodel.cpp",
            "ifunctionhintproposalmodel.h",
            "igenericproposalmodel.cpp",
            "igenericproposalmodel.h",
            "keywordscompletionassist.cpp",
            "keywordscompletionassist.h",
            "quickfixassistprocessor.cpp",
            "quickfixassistprocessor.h",
            "quickfixassistprovider.cpp",
            "quickfixassistprovider.h",
            "runner.cpp",
            "runner.h",
        ]
    }

    Group {
        prefix: "generichighlighter/"
        files: [
            "context.cpp",
            "context.h",
            "definitiondownloader.cpp",
            "definitiondownloader.h",
            "dynamicrule.cpp",
            "dynamicrule.h",
            "highlightdefinition.cpp",
            "highlightdefinition.h",
            "highlightdefinitionhandler.cpp",
            "highlightdefinitionhandler.h",
            "highlightdefinitionmetadata.cpp",
            "highlightdefinitionmetadata.h",
            "highlighter.cpp",
            "highlighter.h",
            "highlighterexception.h",
            "highlightersettings.cpp",
            "highlightersettings.h",
            "highlightersettingspage.cpp",
            "highlightersettingspage.h",
            "highlightersettingspage.ui",
            "includerulesinstruction.cpp",
            "includerulesinstruction.h",
            "itemdata.cpp",
            "itemdata.h",
            "keywordlist.cpp",
            "keywordlist.h",
            "managedefinitionsdialog.cpp",
            "managedefinitionsdialog.h",
            "managedefinitionsdialog.ui",
            "manager.cpp",
            "manager.h",
            "progressdata.cpp",
            "progressdata.h",
            "reuse.h",
            "rule.cpp",
            "rule.h",
            "specificrules.cpp",
            "specificrules.h",
        ]
    }

    Group {
        prefix: "snippets/"
        files: [
            "isnippetprovider.cpp",
            "isnippetprovider.h",
            "plaintextsnippetprovider.cpp",
            "plaintextsnippetprovider.h",
            "reuse.h",
            "snippet.cpp",
            "snippet.h",
            "snippetassistcollector.cpp",
            "snippetassistcollector.h",
            "snippeteditor.cpp",
            "snippeteditor.h",
            "snippetscollection.cpp",
            "snippetscollection.h",
            "snippetssettings.cpp",
            "snippetssettings.h",
            "snippetssettingspage.cpp",
            "snippetssettingspage.h",
            "snippetssettingspage.ui",
        ]
    }

    Export {
        Depends { name: "Find" }
        Depends { name: "Locator" }
    }
}
