#include <QtWebKit>
#include <QRectF>

#if QT_VERSION >= 0x050000
#include <QtWebKitWidgets>
#endif

class CutyNetworkAccessManager : public QNetworkAccessManager {
  Q_OBJECT

public:
  CutyNetworkAccessManager();
  void setAllowRemoteResources(bool allowRemoteResources);
  QNetworkReply * createRequest(Operation op, const QNetworkRequest & req, QIODevice * outgoingData = 0);

protected:
  bool mAllowRemoteResources;
};

class CutyCapt;
class CutyPage : public QWebPage {
  Q_OBJECT

public:
  void setAttribute(QWebSettings::WebAttribute option, const QString& value);
  void setUserAgent(const QString& userAgent);
  void setAlertString(const QString& alertString);
  void setPrintAlerts(bool printAlerts);
  void setCutyCapt(CutyCapt* cutyCapt);
  QString getAlertString();

protected:
  QString chooseFile(QWebFrame *frame, const QString& suggestedFile);
  void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID);
  bool javaScriptPrompt(QWebFrame* frame, const QString& msg, const QString& defaultValue, QString* result);
  void javaScriptAlert(QWebFrame* frame, const QString& msg);
  bool javaScriptConfirm(QWebFrame* frame, const QString& msg);
  QString userAgentForUrl(const QUrl& url) const;
  QString mUserAgent;
  QString mAlertString;
  bool mPrintAlerts;
  CutyCapt* mCutyCapt;
};

class CutyCapt : public QObject {
  Q_OBJECT

public:

  // TODO: This should really be elsewhere and be named differently
  enum OutputFormat { SvgFormat, PdfFormat, PsFormat, InnerTextFormat, HtmlFormat,
    RenderTreeFormat, PngFormat, JpegFormat, MngFormat, TiffFormat, GifFormat,
    BmpFormat, PpmFormat, XbmFormat, XpmFormat, OtherFormat };

  CutyCapt(CutyPage* page,
           const QString& output,
           int delay,
           OutputFormat format,
           const QString& scriptProp,
           const QString& scriptCode,
           bool insecure,
           bool smooth,
		   int pageWidth,
		   int pageHeight,
           QRectF margins);

private slots:
  void DocumentComplete(bool ok);
  void InitialLayoutCompleted();
  void JavaScriptWindowObjectCleared();
  void Timeout();
  void Delayed();
  void handleSslErrors(QNetworkReply* reply, QList<QSslError> errors);

private:
  void TryDelayedRender();
  void saveSnapshot();
  bool mSawInitialLayout;
  bool mSawDocumentComplete;

protected:
  QString      mOutput;
  int          mDelay;
  CutyPage*    mPage;
  OutputFormat mFormat;
  QObject*     mScriptObj;
  QString      mScriptProp;
  QString      mScriptCode;
  bool         mInsecure;
  bool         mSmooth;
  int          mPageWidth;
  int          mPageHeight;
  QRectF       mMargins;
};
