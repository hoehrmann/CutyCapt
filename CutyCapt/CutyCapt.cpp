////////////////////////////////////////////////////////////////////
//
// CutyCapt - A Qt WebKit Web Page Rendering Capture Utility
//
// Copyright (C) 2003-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Id$
//
////////////////////////////////////////////////////////////////////

#include <QApplication>
#include <QtWebKit>
#include <QtGui>
#include <QSvgGenerator>
#include <QPrinter>
#include <QTimer>
#include <QByteArray>
#include <QNetworkRequest>
#include "CutyCapt.hpp"

#if QT_VERSION >= 0x040600 && 0
#define CUTYCAPT_SCRIPT 1
#endif

#ifdef STATIC_PLUGINS
  Q_IMPORT_PLUGIN(qjpeg)
  Q_IMPORT_PLUGIN(qgif)
  Q_IMPORT_PLUGIN(qtiff)
  Q_IMPORT_PLUGIN(qsvg)
  Q_IMPORT_PLUGIN(qmng)
  Q_IMPORT_PLUGIN(qico)
#endif

static struct _CutyExtMap {
  CutyCapt::OutputFormat id;
  const char* extension;
  const char* identifier;
} const CutyExtMap[] = {
  { CutyCapt::SvgFormat,         ".svg",        "svg"   },
  { CutyCapt::PdfFormat,         ".pdf",        "pdf"   },
  { CutyCapt::PsFormat,          ".ps",         "ps"    },
  { CutyCapt::InnerTextFormat,   ".txt",        "itext" },
  { CutyCapt::HtmlFormat,        ".html",       "html"  },
  { CutyCapt::RenderTreeFormat,  ".rtree",      "rtree" },
  { CutyCapt::JpegFormat,        ".jpeg",       "jpeg"  },
  { CutyCapt::PngFormat,         ".png",        "png"   },
  { CutyCapt::MngFormat,         ".mng",        "mng"   },
  { CutyCapt::TiffFormat,        ".tiff",       "tiff"  },
  { CutyCapt::GifFormat,         ".gif",        "gif"   },
  { CutyCapt::BmpFormat,         ".bmp",        "bmp"   },
  { CutyCapt::PpmFormat,         ".ppm",        "ppm"   },
  { CutyCapt::XbmFormat,         ".xbm",        "xbm"   },
  { CutyCapt::XpmFormat,         ".xpm",        "xpm"   },
  { CutyCapt::OtherFormat,       "",            ""      }
};

QString
CutyPage::chooseFile(QWebFrame* /*frame*/, const QString& /*suggestedFile*/) {
  return QString::null;
}

bool
CutyPage::javaScriptConfirm(QWebFrame* /*frame*/, const QString& /*msg*/) {
  return true;
}

bool
CutyPage::javaScriptPrompt(QWebFrame* /*frame*/,
                           const QString& /*msg*/,
                           const QString& /*defaultValue*/,
                           QString* /*result*/) {
  return true;
}

void
CutyPage::javaScriptConsoleMessage(const QString& /*message*/,
                                   int /*lineNumber*/,
                                   const QString& /*sourceID*/) {
  // noop
}

void
CutyPage::javaScriptAlert(QWebFrame* /*frame*/, const QString& msg) {

  if (mPrintAlerts)
    qDebug() << "[alert]" << msg;

  if (mAlertString == msg) {
    QTimer::singleShot(10, mCutyCapt, SLOT(Delayed()));
  }
}

QString
CutyPage::userAgentForUrl(const QUrl& url) const {

  if (!mUserAgent.isNull())
    return mUserAgent;

  return QWebPage::userAgentForUrl(url);
}

void
CutyPage::setUserAgent(const QString& userAgent) {
  mUserAgent = userAgent;
}

void
CutyPage::setAlertString(const QString& alertString) {
  mAlertString = alertString;
}

QString
CutyPage::getAlertString() {
  return mAlertString;
}

void
CutyPage::setCutyCapt(CutyCapt* cutyCapt) {
  mCutyCapt = cutyCapt;
}

void
CutyPage::setPrintAlerts(bool printAlerts) {
  mPrintAlerts = printAlerts;
}

void
CutyPage::setAttribute(QWebSettings::WebAttribute option,
                       const QString& value) {

  if (value == "on")
    settings()->setAttribute(option, true);
  else if (value == "off")
    settings()->setAttribute(option, false);
  else
    (void)0; // TODO: ...
}

// TODO: Consider merging some of main() and CutyCap

CutyCapt::CutyCapt(CutyPage* page, const QString& output, int delay, OutputFormat format,
                   const QString& scriptProp, const QString& scriptCode) {
  mPage = page;
  mOutput = output;
  mDelay = delay;
  mSawInitialLayout = false;
  mSawDocumentComplete = false;
  mFormat = format;
  mScriptProp = scriptProp;
  mScriptCode = scriptCode;
  mScriptObj = new QObject();

  // This is not really nice, but some restructuring work is
  // needed anyway, so this should not be that bad for now.
  mPage->setCutyCapt(this);
}

void
CutyCapt::InitialLayoutCompleted() {
  mSawInitialLayout = true;

  if (mSawInitialLayout && mSawDocumentComplete)
    TryDelayedRender();
}

void
CutyCapt::DocumentComplete(bool /*ok*/) {
  mSawDocumentComplete = true;

  if (mSawInitialLayout && mSawDocumentComplete)
    TryDelayedRender();
}

void
CutyCapt::JavaScriptWindowObjectCleared() {

  if (!mScriptProp.isEmpty()) {
    QVariant var = mPage->mainFrame()->evaluateJavaScript(mScriptProp);
    QObject* obj = var.value<QObject*>();

    if (obj == mScriptObj)
      return;

    mPage->mainFrame()->addToJavaScriptWindowObject(mScriptProp, mScriptObj);
  }

  mPage->mainFrame()->evaluateJavaScript(mScriptCode);

}

void
CutyCapt::TryDelayedRender() {

  if (!mPage->getAlertString().isEmpty())
    return;

  if (mDelay > 0) {
    QTimer::singleShot(mDelay, this, SLOT(Delayed()));
    return;
  }

  saveSnapshot();
  QApplication::exit();
}

void
CutyCapt::Timeout() {
  saveSnapshot();
  QApplication::exit();
}

void
CutyCapt::Delayed() {
  saveSnapshot();
  QApplication::exit();
}

void
CutyCapt::saveSnapshot() {
  QWebFrame *mainFrame = mPage->mainFrame();
  QPainter painter;
  const char* format = NULL;

  for (int ix = 0; CutyExtMap[ix].id != OtherFormat; ++ix)
    if (CutyExtMap[ix].id == mFormat)
      format = CutyExtMap[ix].identifier; //, break;

  // TODO: sometimes contents/viewport can have size 0x0
  // in which case saving them will fail. This is likely
  // the result of the method being called too early. So
  // far I've been unable to find a workaround, except
  // using --delay with some substantial wait time. I've
  // tried to resize multiple time, make a fake render,
  // check for other events... This is primarily a problem
  // under my Ubuntu virtual machine.

  mPage->setViewportSize( mainFrame->contentsSize() );

  switch (mFormat) {
    case SvgFormat: {
      QSvgGenerator svg;
      svg.setFileName(mOutput);
      svg.setSize(mPage->viewportSize());
      painter.begin(&svg);
      mainFrame->render(&painter);
      painter.end();
      break;
    }
    case PdfFormat:
    case PsFormat: {
      QPrinter printer;
      printer.setPageSize(QPrinter::A4);
      printer.setOutputFileName(mOutput);
      // TODO: change quality here?
      mainFrame->print(&printer);
      break;
    }
    case RenderTreeFormat:
    case InnerTextFormat:
    case HtmlFormat: {
      QFile file(mOutput);
      file.open(QIODevice::WriteOnly | QIODevice::Text);
      QTextStream s(&file);
      s.setCodec("utf-8");
      s << (mFormat == RenderTreeFormat ? mainFrame->renderTreeDump() :
            mFormat == InnerTextFormat  ? mainFrame->toPlainText() :
            mFormat == HtmlFormat       ? mainFrame->toHtml() :
            "bug");
      break;
    }
    default: {
      QImage image(mPage->viewportSize(), QImage::Format_ARGB32);
      painter.begin(&image);
      mainFrame->render(&painter);
      painter.end();
      // TODO: add quality
      image.save(mOutput, format);
    }
  };
}

void
CaptHelp(void) {
  printf("%s",
    " -----------------------------------------------------------------------------\n"
    " Usage: CutyCapt --url=http://www.example.org/ --out=localfile.png            \n"
    " -----------------------------------------------------------------------------\n"
    "  --help                         Print this help page and exit                \n"
    "  --url=<url>                    The URL to capture (http:...|file:...|...)   \n"
    "  --out=<path>                   The target file (.png|pdf|ps|svg|jpeg|...)   \n"
    "  --out-format=<f>               Like extension in --out, overrides heuristic \n"
//  "  --out-quality=<int>            Output format quality from 1 to 100          \n"
    "  --min-width=<int>              Minimal width for the image (default: 800)   \n"
    "  --min-height=<int>             Minimal height for the image (default: 600)  \n"
    "  --max-wait=<ms>                Don't wait more than (default: 90000, inf: 0)\n"
    "  --delay=<ms>                   After successful load, wait (default: 0)     \n"
//  "  --user-styles=<url>            Location of user style sheet (deprecated)    \n"
    "  --user-style-path=<path>       Location of user style sheet file, if any    \n"
    "  --user-style-string=<css>      User style rules specified as text           \n"
    "  --header=<name>:<value>        request header; repeatable; some can't be set\n"
    "  --method=<get|post|put>        Specifies the request method (default: get)  \n"
    "  --body-string=<string>         Unencoded request body (default: none)       \n"
    "  --body-base64=<base64>         Base64-encoded request body (default: none)  \n"
    "  --app-name=<name>              appName used in User-Agent; default is none  \n"
    "  --app-version=<version>        appVers used in User-Agent; default is none  \n"
    "  --user-agent=<string>          Override the User-Agent header Qt would set  \n"
    "  --javascript=<on|off>          JavaScript execution (default: on)           \n"
    "  --java=<on|off>                Java execution (default: unknown)            \n"
    "  --plugins=<on|off>             Plugin execution (default: unknown)          \n"
    "  --private-browsing=<on|off>    Private browsing (default: unknown)          \n"
    "  --auto-load-images=<on|off>    Automatic image loading (default: on)        \n"
    "  --js-can-open-windows=<on|off> Script can open windows? (default: unknown)  \n"
    "  --js-can-access-clipboard=<on|off> Script clipboard privs (default: unknown)\n"
#if QT_VERSION >= 0x040500
    "  --print-backgrounds=<on|off>   Backgrounds in PDF/PS output (default: off)  \n"
    "  --zoom-factor=<float>          Page zoom factor (default: no zooming)       \n"
    "  --zoom-text-only=<on|off>      Whether to zoom only the text (default: off) \n"
    "  --http-proxy=<url>             Address for HTTP proxy server (default: none)\n"
#endif
#if CUTYCAPT_SCRIPT
    "  --inject-script=<path>         JavaScript that will be injected into pages  \n"
    "  --script-object=<string>       Property to hold state for injected script   \n"
    "  --expect-alert=<string>        Try waiting for alert(string) before capture \n"
    "  --debug-print-alerts           Prints out alert(...) strings for debugging. \n"
#endif
    " -----------------------------------------------------------------------------\n"
    "  <f> is svg,ps,pdf,itext,html,rtree,png,jpeg,mng,tiff,gif,bmp,ppm,xbm,xpm    \n"
    " -----------------------------------------------------------------------------\n"
#if CUTYCAPT_SCRIPT
    " The `inject-script` option can be used to inject script code into loaded web \n"
    " pages. The code is called whenever the `javaScriptWindowObjectCleared` signal\n"
    " is received. When `script-object` is set, an object under the specified name \n"
    " will be available to the script to maintain state across page loads. When the\n"
    " `expect-alert` option is specified, the shot will be taken when a script in- \n"
    " vokes alert(string) with the string if that happens before `max-wait`. These \n"
    " options effectively allow you to remote control the browser and the web page.\n"
    " This an experimental and easily abused and misused feature. Use with caution.\n"
    " -----------------------------------------------------------------------------\n"
#endif
    " http://cutycapt.sf.net - (c) 2003-2010 Bjoern Hoehrmann - bjoern@hoehrmann.de\n"
    "");
}

int
main(int argc, char *argv[]) {

  int argHelp = 0;
  int argDelay = 0;
  int argSilent = 0;
  int argMinWidth = 800;
  int argMinHeight = 600;
  int argMaxWait = 90000;
  int argVerbosity = 0;
  
  const char* argUrl = NULL;
  const char* argUserStyle = NULL;
  const char* argUserStylePath = NULL;
  const char* argUserStyleString = NULL;
  const char* argIconDbPath = NULL;
  const char* argInjectScript = NULL;
  const char* argScriptObject = NULL;
  QString argOut;

  CutyCapt::OutputFormat format = CutyCapt::OtherFormat;

  QApplication app(argc, argv, true);
  CutyPage page;

  QNetworkAccessManager::Operation method =
    QNetworkAccessManager::GetOperation;
  QByteArray body;
  QNetworkRequest req;
  QNetworkAccessManager manager;

  // Parse command line parameters
  for (int ax = 1; ax < argc; ++ax) {
    size_t nlen;

    const char* s = argv[ax];
    const char* value;

    // boolean options
    if (strcmp("--silent", s) == 0) {
      argSilent = 1;
      continue;

    } else if (strcmp("--help", s) == 0) {
      argHelp = 1;
      break;

    } else if (strcmp("--verbose", s) == 0) {
      argVerbosity++;
      continue;

#if CUTYCAPT_SCRIPT
    } else if (strcmp("--debug-print-alerts", s) == 0) {
      page.setPrintAlerts(true);
      continue;
#endif
    } 

    value = strchr(s, '=');

    if (value == NULL) {
      // TODO: error
      argHelp = 1;
      break;
    }

    nlen = value++ - s;

    // --name=value options
    if (strncmp("--url", s, nlen) == 0) {
      argUrl = value;

    } else if (strncmp("--min-width", s, nlen) == 0) {
      // TODO: add error checking here?
      argMinWidth = (unsigned int)atoi(value);

    } else if (strncmp("--min-height", s, nlen) == 0) {
      // TODO: add error checking here?
      argMinHeight = (unsigned int)atoi(value);

    } else if (strncmp("--delay", s, nlen) == 0) {
      // TODO: see above
      argDelay = (unsigned int)atoi(value);

    } else if (strncmp("--max-wait", s, nlen) == 0) {
      // TODO: see above
      argMaxWait = (unsigned int)atoi(value);

    } else if (strncmp("--out", s, nlen) == 0) {
      argOut = value;

      if (format == CutyCapt::OtherFormat)
        for (int ix = 0; CutyExtMap[ix].id != CutyCapt::OtherFormat; ++ix)
          if (argOut.endsWith(CutyExtMap[ix].extension))
            format = CutyExtMap[ix].id; //, break;

    } else if (strncmp("--user-styles", s, nlen) == 0) {
      // This option is provided for backwards-compatibility only
      argUserStyle = value;

    } else if (strncmp("--user-style-path", s, nlen) == 0) {
      argUserStylePath = value;

    } else if (strncmp("--user-style-string", s, nlen) == 0) {
      argUserStyleString = value;

    } else if (strncmp("--icon-database-path", s, nlen) == 0) {
      argIconDbPath = value;

    } else if (strncmp("--auto-load-images", s, nlen) == 0) {
      page.setAttribute(QWebSettings::AutoLoadImages, value);

    } else if (strncmp("--javascript", s, nlen) == 0) {
      page.setAttribute(QWebSettings::JavascriptEnabled, value);

    } else if (strncmp("--java", s, nlen) == 0) {
      page.setAttribute(QWebSettings::JavaEnabled, value);

    } else if (strncmp("--plugins", s, nlen) == 0) {
      page.setAttribute(QWebSettings::PluginsEnabled, value);

    } else if (strncmp("--private-browsing", s, nlen) == 0) {
      page.setAttribute(QWebSettings::PrivateBrowsingEnabled, value);

    } else if (strncmp("--js-can-open-windows", s, nlen) == 0) {
      page.setAttribute(QWebSettings::JavascriptCanOpenWindows, value);

    } else if (strncmp("--js-can-access-clipboard", s, nlen) == 0) {
      page.setAttribute(QWebSettings::JavascriptCanAccessClipboard, value);

    } else if (strncmp("--developer-extras", s, nlen) == 0) {
      page.setAttribute(QWebSettings::DeveloperExtrasEnabled, value);

    } else if (strncmp("--links-included-in-focus-chain", s, nlen) == 0) {
      page.setAttribute(QWebSettings::LinksIncludedInFocusChain, value);

#if QT_VERSION >= 0x040500
    } else if (strncmp("--print-backgrounds", s, nlen) == 0) {
      page.setAttribute(QWebSettings::PrintElementBackgrounds, value);

    } else if (strncmp("--zoom-factor", s, nlen) == 0) {
      page.mainFrame()->setZoomFactor(QString(value).toFloat());

    } else if (strncmp("--zoom-text-only", s, nlen) == 0) {
      page.setAttribute(QWebSettings::ZoomTextOnly, value);

    } else if (strncmp("--http-proxy", s, nlen) == 0) {
      QUrl p = QUrl::fromEncoded(value);
      QNetworkProxy proxy = QNetworkProxy(QNetworkProxy::HttpProxy,
        p.host(), p.port(80), p.userName(), p.password());
      manager.setProxy(proxy);
      page.setNetworkAccessManager(&manager);
#endif

#if CUTYCAPT_SCRIPT
    } else if (strncmp("--inject-script", s, nlen) == 0) {
      argInjectScript = value;

    } else if (strncmp("--script-object", s, nlen) == 0) {
      argScriptObject = value;

    } else if (strncmp("--expect-alert", s, nlen) == 0) {
      page.setAlertString(value);
#endif

    } else if (strncmp("--app-name", s, nlen) == 0) {
      app.setApplicationName(value);

    } else if (strncmp("--app-version", s, nlen) == 0) {
      app.setApplicationVersion(value);

    } else if (strncmp("--body-base64", s, nlen) == 0) {
      body = QByteArray::fromBase64(value);

    } else if (strncmp("--body-string", s, nlen) == 0) {
      body = QByteArray(value);

    } else if (strncmp("--user-agent", s, nlen) == 0) {
      page.setUserAgent(value);

    } else if (strncmp("--out-format", s, nlen) == 0) {
      for (int ix = 0; CutyExtMap[ix].id != CutyCapt::OtherFormat; ++ix)
        if (strcmp(value, CutyExtMap[ix].identifier) == 0)
          format = CutyExtMap[ix].id; //, break;

      if (format == CutyCapt::OtherFormat) {
        // TODO: error
        argHelp = 1;
        break;
      }

    } else if (strncmp("--header", s, nlen) == 0) {
      const char* hv = strchr(value, ':');

      if (hv == NULL) {
        // TODO: error
        argHelp = 1;
        break;
      }

      req.setRawHeader(QByteArray(value, hv - value), hv + 1);

    } else if (strncmp("--method", s, nlen) == 0) {
      if (strcmp("value", "get") == 0)
        method = QNetworkAccessManager::GetOperation;
      else if (strcmp("value", "put") == 0)
        method = QNetworkAccessManager::PutOperation;
      else if (strcmp("value", "post") == 0)
        method = QNetworkAccessManager::PostOperation;
      else if (strcmp("value", "head") == 0)
        method = QNetworkAccessManager::HeadOperation;
      else 
        (void)0; // TODO: ...

    } else {
      // TODO: error
      argHelp = 1;
    }
  }

  if (argUrl == NULL || argOut == NULL || argHelp) {
      CaptHelp();
      return EXIT_FAILURE;
  }

  // This used to use QUrl(argUrl) but that escapes %hh sequences
  // even though it should not, as URLs can assumed to be escaped.
  req.setUrl( QUrl::fromEncoded(argUrl) );

  QString scriptProp(argScriptObject);
  QString scriptCode;

  if (argInjectScript) {
    QFile file(argInjectScript);
    if (file.open(QIODevice::ReadOnly)) {
      QTextStream stream(&file);
      stream.setCodec(QTextCodec::codecForName("UTF-8"));
      stream.setAutoDetectUnicode(true);
      scriptCode = stream.readAll();
      file.close();
    }
  }

  CutyCapt main(&page, argOut, argDelay, format, scriptProp, scriptCode);

  app.connect(&page,
    SIGNAL(loadFinished(bool)),
    &main,
    SLOT(DocumentComplete(bool)));

  app.connect(page.mainFrame(),
    SIGNAL(initialLayoutCompleted()),
    &main,
    SLOT(InitialLayoutCompleted()));

  if (argMaxWait > 0) {
    // TODO: Should this also register one for the application?
    QTimer::singleShot(argMaxWait, &main, SLOT(Timeout()));
  }

  if (argUserStyle != NULL)
    // TODO: does this need any syntax checking?
    page.settings()->setUserStyleSheetUrl( QUrl::fromEncoded(argUserStyle) );

  if (argUserStylePath != NULL) {
    page.settings()->setUserStyleSheetUrl( QUrl::fromLocalFile(argUserStylePath) );
  }

  if (argUserStyleString != NULL) {
    QUrl data("data:text/css;charset=utf-8;base64," +
      QByteArray(argUserStyleString).toBase64());
    page.settings()->setUserStyleSheetUrl( data );
  }

  if (argIconDbPath != NULL)
    // TODO: does this need any syntax checking?
    page.settings()->setIconDatabasePath(argUserStyle);

  // The documentation does not say, but it seems the mainFrame
  // will never change, so we can set this here. Otherwise we'd
  // have to set this in snapshot and trigger an update, which
  // is not currently possible (Qt 4.4.0) as far as I can tell.
  page.mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
  page.mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
  page.setViewportSize( QSize(argMinWidth, argMinHeight) );

#if CUTYCAPT_SCRIPT
  // javaScriptWindowObjectCleared does not get called on the
  // initial load unless some JavaScript has been executed.
  page.mainFrame()->evaluateJavaScript(QString(""));

  app.connect(page.mainFrame(),
    SIGNAL(javaScriptWindowObjectCleared()),
    &main,
    SLOT(JavaScriptWindowObjectCleared()));
#endif

  if (!body.isNull())
    page.mainFrame()->load(req, method, body);
  else
    page.mainFrame()->load(req, method);

  return app.exec();
}
