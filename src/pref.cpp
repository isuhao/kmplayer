/**
 * Copyright (C) 2003 Joonas Koivunen <rzei@mbnet.fi>
 * Copyright (C) 2003 Koos Vriezen <koos.vriezen@xs4all.nl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#undef Always

#include <algorithm>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <qlayout.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qtable.h>
#include <qstringlist.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qgroupbox.h>
#include <qtooltip.h>
#include <qtabwidget.h>
#include <qslider.h>
#include <qbuttongroup.h>
#include <qspinbox.h>
#include <qmessagebox.h>
#include <qmap.h>

#include <klocale.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <klineedit.h>
#include <kiconloader.h>
#include <kdeversion.h>
#include <kcombobox.h>
#include <kurlrequester.h>

#include "pref.h"
#include "kmplayerpartbase.h"
#include "kmplayerprocess.h"
#include "kmplayerconfig.h"


KMPlayerPreferences::KMPlayerPreferences(KMPlayer * player, KMPlayerSettings * settings)
: KDialogBase (IconList, i18n ("KMPlayer Preferences"),
		Help|Default|Ok|Apply|Cancel, Ok, player->view (), 0, false)
{
    QFrame *frame;
    QTabWidget * tab;
    QStringList hierarchy; // typo? :)
    QVBoxLayout *vlay;
    QMap<QString, QTabWidget *> entries;

    frame = addPage(i18n("General Options"), QString::null, KGlobal::iconLoader()->loadIcon (QString ("kmplayer"), KIcon::NoGroup, 32));
    vlay = new QVBoxLayout(frame, marginHint(), spacingHint());
    tab = new QTabWidget (frame);
    vlay->addWidget (tab);
    m_GeneralPageGeneral = new KMPlayerPrefGeneralPageGeneral (tab);
    tab->insertTab (m_GeneralPageGeneral, i18n("General"));
    m_GeneralPageOutput = new KMPlayerPrefGeneralPageOutput
        (tab, settings->audiodrivers, settings->videodrivers);
    tab->insertTab (m_GeneralPageOutput, i18n("Output"));
    entries.insert (i18n("General Options"), tab);

    frame = addPage (i18n ("Source"), QString::null, KGlobal::iconLoader()->loadIcon (QString ("source"), KIcon::NoGroup, 32));
    vlay = new QVBoxLayout (frame, marginHint(), spacingHint());
    tab = new QTabWidget (frame);
    vlay->addWidget (tab);
    m_SourcePageURL = new KMPlayerPrefSourcePageURL (tab);
    tab->insertTab (m_SourcePageURL, i18n ("URL"));
    entries.insert (i18n("Source"), tab);

    frame = addPage (i18n ("Recording"), QString::null, KGlobal::iconLoader()->loadIcon (QString ("video"), KIcon::NoGroup, 32));
    vlay = new QVBoxLayout (frame, marginHint(), spacingHint());
    tab = new QTabWidget (frame);
    vlay->addWidget (tab);
    m_MEncoderPage = new KMPlayerPrefMEncoderPage (tab, player);
    tab->insertTab (m_MEncoderPage, i18n ("MEncoder"));
    recorders.push_back (m_MEncoderPage);
    m_FFMpegPage = new KMPlayerPrefFFMpegPage (tab, player);
    tab->insertTab (m_FFMpegPage, i18n ("FFMpeg"));
    recorders.push_back (m_FFMpegPage);
    m_RecordPage = new KMPlayerPrefRecordPage (tab, player, recorders);
    tab->insertTab (m_RecordPage, i18n ("General"), 0);
    tab->setCurrentPage (0);
    entries.insert (i18n("Recording"), tab);

    frame = addPage (i18n ("Output plugins"), QString::null, KGlobal::iconLoader()->loadIcon (QString ("image"), KIcon::NoGroup, 32));
    vlay = new QVBoxLayout(frame, marginHint(), spacingHint());
    tab = new QTabWidget (frame);
    vlay->addWidget (tab);
    m_OPPagePostproc = new KMPlayerPrefOPPagePostProc (tab);
    tab->insertTab (m_OPPagePostproc, i18n ("Postprocessing"));
    entries.insert (i18n("Postprocessing"), tab);

    KMPlayerPreferencesPageList::iterator pl_it = settings->pagelist.begin ();
    for (; pl_it != settings->pagelist.end (); ++pl_it) {
        QString item, subitem, icon;
        (*pl_it)->prefLocation (item, icon, subitem);
        if (item.isEmpty ())
            continue;
        QMap<QString, QTabWidget *>::iterator en_it = entries.find (item);
        if (en_it == entries.end ()) {
            frame = addPage (item, QString::null, KGlobal::iconLoader()->loadIcon ((icon), KIcon::NoGroup, 32));
            vlay = new QVBoxLayout (frame, marginHint(), spacingHint());
            tab = new QTabWidget (frame);
            vlay->addWidget (tab);
            entries.insert (item, tab);
        } else
            tab = en_it.data ();
        frame = (*pl_it)->prefPage (tab);
        tab->insertTab (frame, subitem);
    }

    connect (this, SIGNAL (defaultClicked ()), SLOT (confirmDefaults ()));
}

void KMPlayerPreferences::setPage (const char * name) {
    QObject * o = child (name, "QFrame");
    if (!o) return;
    QFrame * page = static_cast <QFrame *> (o);
    QWidget * w = page->parentWidget ();
    while (w && !w->inherits ("QTabWidget"))
        w = w->parentWidget ();
    if (!w) return;
    QTabWidget * t = static_cast <QTabWidget*> (w);
    t->setCurrentPage (t->indexOf(page));
    if (!t->parentWidget() || !t->parentWidget()->inherits ("QFrame"))
        return;
    showPage (pageIndex (t->parentWidget ()));
}

KMPlayerPreferences::~KMPlayerPreferences() {
}

KMPlayerPrefGeneralPageGeneral::KMPlayerPrefGeneralPageGeneral(QWidget *parent)
: QFrame (parent, "GeneralPage")
{
	QVBoxLayout *layout = new QVBoxLayout(this, 5, 2);

	keepSizeRatio = new QCheckBox (i18n("Keep size ratio"), this, 0);
	QToolTip::add(keepSizeRatio, i18n("When checked, movie will keep its aspect ratio\nwhen window is resized"));
	showConsoleOutput = new QCheckBox (i18n("Show console output"), this, 0);
	QToolTip::add(showConsoleOutput, i18n("Shows output from mplayer before and after playing the movie"));
	loop = new QCheckBox (i18n("Loop"), this, 0);
	QToolTip::add(loop, i18n("Makes current movie loop"));
	showControlButtons = new QCheckBox (i18n("Show control buttons"), this, 0);
	QToolTip::add(showControlButtons, i18n("Small buttons will be shown above statusbar to control movie"));
	autoHideControlButtons = new QCheckBox (i18n("Auto hide control buttons"), this, 0);
	QToolTip::add(autoHideControlButtons, i18n("When checked, control buttons will get hidden automatically"));
	showPositionSlider	= new QCheckBox (i18n("Show position slider"), this, 0);
	QToolTip::add(showPositionSlider, i18n("When enabled, will show a seeking slider under the control buttons"));
	showRecordButton = new QCheckBox (i18n ("Show record button"), this);
	QToolTip::add (showRecordButton, i18n ("Add a record button to the control buttons"));
	showBroadcastButton = new QCheckBox (i18n ("Show broadcast button"), this);
	QToolTip::add (showBroadcastButton, i18n ("Add a broadcast button to the control buttons"));
	//autoHideSlider = new QCheckBox (i18n("Auto hide position slider"), this, 0);
	alwaysBuildIndex = new QCheckBox ( i18n("Build new index when possible"), this);
	QToolTip::add(alwaysBuildIndex, i18n("Allows seeking in indexed files (AVIs)"));
	framedrop = new QCheckBox (i18n ("Allow framedrops"), this);
	QToolTip::add (framedrop, i18n ("Allow dropping frames for better audio and video synchronization"));

	QWidget *seekingWidget = new QWidget(this);
	QHBoxLayout *seekingWidgetLayout = new QHBoxLayout(seekingWidget);
	seekingWidgetLayout->addWidget(new QLabel(i18n("Forward/backward seek time:"),seekingWidget));
	seekingWidgetLayout->addItem(new QSpacerItem(0,0,QSizePolicy::Minimum, QSizePolicy::Minimum));
	seekTime = new QSpinBox(1, 600, 1, seekingWidget);
	seekingWidgetLayout->addWidget(seekTime);
	seekingWidgetLayout->addItem(new QSpacerItem(0,0,QSizePolicy::Minimum, QSizePolicy::Minimum));
	layout->addWidget(keepSizeRatio);
	layout->addWidget(showConsoleOutput);
	layout->addWidget(loop);
	layout->addWidget (framedrop);
	layout->addWidget(showControlButtons);
	layout->addWidget(autoHideControlButtons);
	layout->addWidget(showPositionSlider);
	layout->addWidget (showRecordButton);
	layout->addWidget (showBroadcastButton);
	//layout->addWidget(autoHideSlider);
	layout->addWidget(alwaysBuildIndex);
	layout->addItem (new QSpacerItem (0, 5));
	layout->addWidget(seekingWidget);
        layout->addItem (new QSpacerItem (0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

KMPlayerPrefSourcePageURL::KMPlayerPrefSourcePageURL (QWidget *parent)
: QFrame (parent, "URLPage")
{
    QVBoxLayout *layout = new QVBoxLayout (this, 5, 5);
    QHBoxLayout * urllayout = new QHBoxLayout ();
    QHBoxLayout * sub_urllayout = new QHBoxLayout ();
    QLabel *urlLabel = new QLabel (i18n ("Location:"), this, 0);
    urllist = new KComboBox (true, this);
    urllist->setMaxCount (20);
    urllist->setDuplicatesEnabled (false); // not that it helps much :(
    url = new KURLRequester (urllist, this);
    //url->setShowLocalProtocol (true);
    url->setSizePolicy (QSizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred));
    QLabel *sub_urlLabel = new QLabel (i18n ("Sub Title:"), this, 0);
    sub_urllist = new KComboBox (true, this);
    sub_urllist->setMaxCount (20);
    sub_urllist->setDuplicatesEnabled (false); // not that it helps much :(
    sub_url = new KURLRequester (sub_urllist, this);
    sub_url->setSizePolicy (QSizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred));
    backend = new QListBox (this);
    backend->insertItem (QString ("MPlayer"), 0);
    backend->insertItem (QString ("Xine"), 1);
    allowhref = new QCheckBox (i18n ("Enable 'Click to Play' support"), this);
    layout->addWidget (allowhref);
    urllayout->addWidget (urlLabel);
    urllayout->addWidget (url);
    layout->addLayout (urllayout);
    sub_urllayout->addWidget (sub_urlLabel);
    sub_urllayout->addWidget (sub_url);
    layout->addLayout (sub_urllayout);
    layout->addItem (new QSpacerItem (0, 10, QSizePolicy::Minimum, QSizePolicy::Minimum));
#ifdef HAVE_XINE
    QGridLayout * gridlayout = new QGridLayout (2, 2);
    QLabel *backendLabel = new QLabel (i18n ("Use Movie Player:"), this, 0);
    //QToolTip::add (allowhref, i18n ("Explain this in a few lines"));
    gridlayout->addWidget (backendLabel, 0, 0);
    gridlayout->addWidget (backend, 1, 0);
    gridlayout->addMultiCell (new QSpacerItem (0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 1, 1, 1);
    layout->addLayout (gridlayout);
#else
    backend->hide ();
#endif
    layout->addItem (new QSpacerItem (0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    connect (urllist, SIGNAL(textChanged (const QString &)),
             this, SLOT (slotTextChanged (const QString &)));
    connect (sub_urllist, SIGNAL(textChanged (const QString &)),
             this, SLOT (slotTextChanged (const QString &)));
}

void KMPlayerPrefSourcePageURL::slotBrowse () {
}

void KMPlayerPrefSourcePageURL::slotTextChanged (const QString &) {
    changed = true;
}

KMPlayerPrefRecordPage::KMPlayerPrefRecordPage (QWidget *parent, KMPlayer * player, RecorderList & rl) : QFrame (parent, "RecordPage"), m_player (player), m_recorders (rl) {
    QVBoxLayout *layout = new QVBoxLayout (this, 5, 5);
    QHBoxLayout * urllayout = new QHBoxLayout ();
    QLabel *urlLabel = new QLabel (i18n ("Output File:"), this);
    url = new KURLRequester ("", this);
    url->setShowLocalProtocol (true);
    urllayout->addWidget (urlLabel);
    urllayout->addWidget (url);
    recordButton = new QPushButton (i18n ("Start &Recording"), this);
    connect (recordButton, SIGNAL (clicked ()), this, SLOT (slotRecord ()));
    QHBoxLayout *buttonlayout = new QHBoxLayout;
    buttonlayout->addItem (new QSpacerItem (0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum));
    buttonlayout->addWidget (recordButton);
    source = new QLabel (i18n ("Current Source: ") + m_player->process ()->source ()->prettyName (), this);
    recorder = new QButtonGroup (m_recorders.size (), Qt::Vertical, i18n ("Recorder"), this);
    RecorderList::iterator it = m_recorders.begin ();
    for (; it != m_recorders.end (); ++it) {
        QRadioButton * radio = new QRadioButton ((*it)->name (), recorder);
        radio->setEnabled ((*it)->sourceSupported (m_player->process ()->source ()));
    }
    recorder->setButton(0); // for now
    replay = new QButtonGroup (4, Qt::Vertical, i18n ("Auto Playback"), this);
    new QRadioButton (i18n ("&No"), replay);
    new QRadioButton (i18n ("&When recording finished"), replay);
    new QRadioButton (i18n ("A&fter"), replay);
    QWidget * customreplay = new QWidget (replay);
    replaytime = new QLineEdit (customreplay);
    QHBoxLayout *replaylayout = new QHBoxLayout (customreplay);
    replaylayout->addWidget (new QLabel (i18n("Time (seconds):"), customreplay));
    replaylayout->addWidget (replaytime);
    replaylayout->addItem (new QSpacerItem (0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    layout->addWidget (source);
    layout->addItem (new QSpacerItem (5, 0, QSizePolicy::Minimum, QSizePolicy::Minimum));
    layout->addLayout (urllayout);
    layout->addItem (new QSpacerItem (5, 0, QSizePolicy::Minimum, QSizePolicy::Minimum));
    layout->addWidget (recorder);
    layout->addItem (new QSpacerItem (5, 0, QSizePolicy::Minimum, QSizePolicy::Minimum));
    layout->addWidget (replay);
    layout->addItem (new QSpacerItem (5, 0, QSizePolicy::Minimum, QSizePolicy::Minimum));
    layout->addLayout (buttonlayout);
    layout->addItem (new QSpacerItem (5, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    connect (m_player, SIGNAL (sourceChanged(KMPlayerSource*)), this, SLOT (sourceChanged(KMPlayerSource*)));
    connect(m_player, SIGNAL(startRecording()), this, SLOT(recordingStarted()));
    connect(m_player, SIGNAL(stopRecording()),this,SLOT (recordingFinished()));
    connect (replay, SIGNAL (clicked (int)), this, SLOT (replayClicked (int)));
}

void KMPlayerPrefRecordPage::recordingStarted () {
    recordButton->setText (i18n ("Stop Recording"));
    url->setEnabled (false);
    topLevelWidget ()->hide ();
}

void KMPlayerPrefRecordPage::recordingFinished () {
    recordButton->setText (i18n ("Start Recording"));
    url->setEnabled (true);
}

void KMPlayerPrefRecordPage::sourceChanged (KMPlayerSource * src) {
    source->setText (i18n ("Current Source: ") + src->prettyName ());
    RecorderList::iterator it = m_recorders.begin ();
    for (int id = 0; it != m_recorders.end (); ++it, ++id) {
        QButton * radio = recorder->find (id);
        radio->setEnabled ((*it)->sourceSupported (src));
    }
}

void KMPlayerPrefRecordPage::replayClicked (int id) {
    replaytime->setEnabled (id == KMPlayerSettings::ReplayAfter);
}

void KMPlayerPrefRecordPage::slotRecord () {
    if (!url->lineEdit()->text().isEmpty()) {
        m_player->stop ();
        m_player->settings ()->recordfile = url->lineEdit()->text();
        m_player->settings ()->replaytime = replaytime->text ().toInt ();
#if KDE_IS_VERSION(3,1,90)
        int id = recorder->selectedId ();
#else
        int id = recorder->id (recorder->selected ());
#endif
        m_player->settings ()->recorder = KMPlayerSettings::Recorder (id);
        RecorderList::iterator it = m_recorders.begin ();
        for (; id > 0 && it != m_recorders.end (); ++it, --id)
            ;
        (*it)->record ();
    }
}

RecorderPage::RecorderPage (QWidget *parent, KMPlayer * player)
 : QFrame (parent), m_player (player) {}

KMPlayerPrefMEncoderPage::KMPlayerPrefMEncoderPage (QWidget *parent, KMPlayer * player) : RecorderPage (parent, player) {
    QVBoxLayout *layout = new QVBoxLayout (this, 5, 5);
    format = new QButtonGroup (3, Qt::Vertical, i18n ("Format"), this);
    new QRadioButton (i18n ("Same as Source"), format);
    new QRadioButton (i18n ("Custom"), format);
    QWidget * customopts = new QWidget (format);
    QGridLayout *gridlayout = new QGridLayout (customopts, 1, 2, 2);
    QLabel *argLabel = new QLabel (i18n("Mencoder arguments:"), customopts, 0);
    arguments = new QLineEdit ("", customopts);
    gridlayout->addWidget (argLabel, 0, 0);
    gridlayout->addWidget (arguments, 0, 1);
    layout->addWidget (format);
    layout->addItem (new QSpacerItem (0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    connect (format, SIGNAL (clicked (int)), this, SLOT (formatClicked (int)));
}

void KMPlayerPrefMEncoderPage::formatClicked (int id) {
    arguments->setEnabled (!!id);
}

void KMPlayerPrefMEncoderPage::record () {
    m_player->setRecorder (m_player->mencoder ());
    if (!m_player->mencoder()->playing ()) {
        m_player->settings ()->mencoderarguments = arguments->text ();
#if KDE_IS_VERSION(3,1,90)
        m_player->settings ()->recordcopy = !format->selectedId ();
#else
        m_player->settings ()->recordcopy = !format->id (format->selected ());
#endif
        m_player->mencoder ()->setURL (KURL (m_player->settings ()->recordfile));
        m_player->mencoder ()->play ();
    } else
        m_player->mencoder ()->stop ();
}

QString KMPlayerPrefMEncoderPage::name () {
    return i18n ("&MEncoder");
}

bool KMPlayerPrefMEncoderPage::sourceSupported (KMPlayerSource *) {
    return true;
}

KMPlayerPrefFFMpegPage::KMPlayerPrefFFMpegPage (QWidget *parent, KMPlayer * player) : RecorderPage (parent, player) {
    QVBoxLayout *layout = new QVBoxLayout (this, 5, 5);
    QGridLayout *gridlayout = new QGridLayout (1, 2, 2);
    QLabel *argLabel = new QLabel (i18n("FFMpeg arguments:"), this);
    arguments = new QLineEdit ("", this);
    gridlayout->addWidget (argLabel, 0, 0);
    gridlayout->addWidget (arguments, 0, 1);
    layout->addLayout (gridlayout);
    layout->addItem (new QSpacerItem (0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void KMPlayerPrefFFMpegPage::record () {
    kdDebug() << "KMPlayerPrefFFMpegPage::record" << endl;
    m_player->setRecorder (m_player->ffmpeg ());
    m_player->ffmpeg ()->setURL (KURL::fromPathOrURL (m_player->settings ()->recordfile));
    m_player->ffmpeg ()->setArguments (arguments->text ());
    m_player->recorder ()->play ();
}

QString KMPlayerPrefFFMpegPage::name () {
    return i18n ("&FFMpeg");
}

bool KMPlayerPrefFFMpegPage::sourceSupported (KMPlayerSource * source) {
    QString protocol = source->url ().protocol ();
    return !source->audioDevice ().isEmpty () ||
           !source->videoDevice ().isEmpty () ||
           !(protocol.startsWith (QString ("dvd")) ||
             protocol.startsWith (QString ("vcd")));
}


KMPlayerPrefGeneralPageOutput::KMPlayerPrefGeneralPageOutput(QWidget *parent, OutputDriver * ad, OutputDriver * vd)
 : QFrame (parent) {
    QGridLayout *layout = new QGridLayout (this, 2, 2, 5);

    videoDriver = new QListBox (this);
    for (int i = 0; vd[i].driver; i++)
        videoDriver->insertItem (vd[i].description, i);
    QToolTip::add(videoDriver, i18n("Sets video driver. Recommended is XVideo, or, if it is not supported, X11, which is slower."));
    layout->addWidget (new QLabel (i18n ("Video driver:"), this), 0, 0);
    layout->addWidget (videoDriver, 1, 0);

    audioDriver = new QListBox (this);
    for (int i = 0; ad[i].driver; i++)
        audioDriver->insertItem (ad[i].description, i);
    layout->addWidget (new QLabel (i18n ("Audio driver:"), this), 0, 1);
    layout->addWidget (audioDriver, 1, 1);
    layout->addItem (new QSpacerItem (0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

KMPlayerPrefOPPageGeneral::KMPlayerPrefOPPageGeneral(QWidget *parent)
: QFrame(parent)
{
    QVBoxLayout *layout = new QVBoxLayout (this, 5);
    layout->setAutoAdd (true);
}

KMPlayerPrefOPPagePostProc::KMPlayerPrefOPPagePostProc(QWidget *parent) : QFrame(parent)
{

	QVBoxLayout *tabLayout = new QVBoxLayout (this, 5);
	postProcessing = new QCheckBox( i18n("postProcessing"), this, 0 );
	postProcessing->setEnabled( TRUE );
	disablePPauto = new QCheckBox (i18n("disablePostProcessingAutomatically"), this, 0);

	tabLayout->addWidget( postProcessing );
	tabLayout->addWidget( disablePPauto );
	tabLayout->addItem ( new QSpacerItem( 5, 5, QSizePolicy::Minimum, QSizePolicy::Minimum ) );

	PostprocessingOptions = new QTabWidget( this, "PostprocessingOptions" );
	PostprocessingOptions->setEnabled( TRUE );
	PostprocessingOptions->setAutoMask( FALSE );
	PostprocessingOptions->setTabPosition( QTabWidget::Top );
	PostprocessingOptions->setTabShape( QTabWidget::Rounded );
	PostprocessingOptions->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, PostprocessingOptions->sizePolicy().hasHeightForWidth() ) );

	QWidget *presetSelectionWidget = new QWidget( PostprocessingOptions, "presetSelectionWidget" );
	QGridLayout *presetSelectionWidgetLayout = new QGridLayout( presetSelectionWidget, 1, 1, 1);

	QButtonGroup *presetSelection = new QButtonGroup(3, Qt::Vertical, presetSelectionWidget);
	presetSelection->setInsideSpacing(KDialog::spacingHint());

	defaultPreset = new QRadioButton( presetSelection, "defaultPreset" );
	defaultPreset->setChecked( TRUE );
	presetSelection->insert (defaultPreset);

	customPreset = new QRadioButton( presetSelection, "customPreset" );
	presetSelection->insert (customPreset);

	fastPreset = new QRadioButton( presetSelection, "fastPreset" );
	presetSelection->insert (fastPreset);
	presetSelection->setRadioButtonExclusive ( true);
	presetSelectionWidgetLayout->addWidget( presetSelection, 0, 0 );
	PostprocessingOptions->insertTab( presetSelectionWidget, "" );

	//
	// SECOND!!!
	//
	/* I JUST WASN'T ABLE TO GET THIS WORKING WITH QGridLayouts */

	QWidget *customFiltersWidget = new QWidget( PostprocessingOptions, "customFiltersWidget" );
	QVBoxLayout *customFiltersWidgetLayout = new QVBoxLayout( customFiltersWidget );

	QGroupBox *customFilters = new QGroupBox(0, Qt::Vertical, customFiltersWidget, "customFilters" );
	customFilters->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)1, (QSizePolicy::SizeType)2));
	customFilters->setFlat(false);
	customFilters->setEnabled( FALSE );
	customFilters->setInsideSpacing(7);

	QLayout *customFiltersLayout = customFilters->layout();
	QHBoxLayout *customFiltersLayout1 = new QHBoxLayout ( customFilters->layout() );

	HzDeblockFilter = new QCheckBox( customFilters, "HzDeblockFilter" );
	HzDeblockAQuality = new QCheckBox( customFilters, "HzDeblockAQuality" );
	HzDeblockAQuality->setEnabled( FALSE );
	HzDeblockCFiltering = new QCheckBox( customFilters, "HzDeblockCFiltering" );
	HzDeblockCFiltering->setEnabled( FALSE );

	customFiltersLayout1->addWidget( HzDeblockFilter );
	customFiltersLayout1->addItem( new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum ) );
	customFiltersLayout1->addWidget( HzDeblockAQuality );
	customFiltersLayout1->addWidget( HzDeblockCFiltering );

	QFrame *line1 = new QFrame( customFilters, "line1" );
	line1->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)2 ) );
	line1->setFrameShape( QFrame::HLine );
	line1->setFrameShadow( QFrame::Sunken );
	customFiltersLayout->add(line1);

	QHBoxLayout *customFiltersLayout2 = new QHBoxLayout ( customFilters->layout() );

	VtDeblockFilter = new QCheckBox( customFilters, "VtDeblockFilter" );
	VtDeblockAQuality = new QCheckBox( customFilters, "VtDeblockAQuality" );
	VtDeblockAQuality->setEnabled( FALSE );
	VtDeblockCFiltering = new QCheckBox( customFilters, "VtDeblockCFiltering" );
	VtDeblockCFiltering->setEnabled( FALSE );

	customFiltersLayout2->addWidget( VtDeblockFilter );
	customFiltersLayout2->addItem( new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum ) );
	customFiltersLayout2->addWidget( VtDeblockAQuality );
	customFiltersLayout2->addWidget( VtDeblockCFiltering );

	QFrame *line2 = new QFrame( customFilters, "line2" );

	line2->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)2 ) );
	line2->setFrameShape( QFrame::HLine );
	line2->setFrameShadow( QFrame::Sunken );
	customFiltersLayout->add(line2);

	QHBoxLayout *customFiltersLayout3  = new QHBoxLayout ( customFilters->layout() );

	DeringFilter = new QCheckBox( customFilters, "DeringFilter" );
	DeringAQuality = new QCheckBox( customFilters, "DeringAQuality" );
	DeringAQuality->setEnabled( FALSE );
	DeringCFiltering = new QCheckBox( customFilters, "DeringCFiltering" );
	DeringCFiltering->setEnabled( FALSE );

	customFiltersLayout3->addWidget( DeringFilter );
	customFiltersLayout3->addItem( new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum ) );
	customFiltersLayout3->addWidget( DeringAQuality );
	customFiltersLayout3->addWidget( DeringCFiltering );


	QFrame *line3 = new QFrame( customFilters, "line3" );

	line3->setFrameShape( QFrame::HLine );
	line3->setFrameShadow( QFrame::Sunken );
	line3->setFrameShape( QFrame::HLine );

	customFiltersLayout->add(line3);

	QHBoxLayout *customFiltersLayout4 = new QHBoxLayout (customFilters->layout());

	AutolevelsFilter = new QCheckBox( customFilters, "AutolevelsFilter" );
	AutolevelsFullrange = new QCheckBox( customFilters, "AutolevelsFullrange" );
	AutolevelsFullrange->setEnabled( FALSE );

	customFiltersLayout4->addWidget(AutolevelsFilter);
	customFiltersLayout4->addItem(new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum ));
	customFiltersLayout4->addWidget(AutolevelsFullrange);

	QHBoxLayout *customFiltersLayout5 = new QHBoxLayout (customFilters->layout());

	TmpNoiseFilter = new QCheckBox( customFilters, "TmpNoiseFilter" );
/*	TmpNoiseSlider = new QSlider( customFilters, "TmpNoiseSlider" );
	TmpNoiseSlider->setEnabled( FALSE );
	TmpNoiseSlider->setMinValue( 1 );
	TmpNoiseSlider->setMaxValue( 3 );
	TmpNoiseSlider->setValue( 1 );
	TmpNoiseSlider->setOrientation( QSlider::Horizontal );
	TmpNoiseSlider->setTickmarks( QSlider::Left );
	TmpNoiseSlider->setTickInterval( 1 );
	TmpNoiseSlider->setSizePolicy(QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1));*/

	/*customFiltersLayout->addWidget(TmpNoiseFilter,7,0);
	customFiltersLayout->addWidget(TmpNoiseSlider,7,2);*/
	customFiltersLayout5->addWidget(TmpNoiseFilter);
	customFiltersLayout5->addItem(new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum ));
	//customFiltersLayout5->addWidget(TmpNoiseSlider);
	customFiltersWidgetLayout->addWidget( customFilters );
	PostprocessingOptions->insertTab( customFiltersWidget, "" );
	//
	//THIRD!!!
	//
	QWidget *deintSelectionWidget = new QWidget( PostprocessingOptions, "deintSelectionWidget" );
	QVBoxLayout *deintSelectionWidgetLayout = new QVBoxLayout( deintSelectionWidget);
	QButtonGroup *deinterlacingGroup = new QButtonGroup(5, Qt::Vertical, deintSelectionWidget, "deinterlacingGroup" );

	LinBlendDeinterlacer = new QCheckBox( deinterlacingGroup, "LinBlendDeinterlacer" );
	LinIntDeinterlacer = new QCheckBox( deinterlacingGroup, "LinIntDeinterlacer" );
	CubicIntDeinterlacer = new QCheckBox( deinterlacingGroup, "CubicIntDeinterlacer" );
	MedianDeinterlacer = new QCheckBox( deinterlacingGroup, "MedianDeinterlacer" );
	FfmpegDeinterlacer = new QCheckBox( deinterlacingGroup, "FfmpegDeinterlacer" );

	deinterlacingGroup->insert( LinBlendDeinterlacer );

	deinterlacingGroup->insert( LinIntDeinterlacer );

	deinterlacingGroup->insert( CubicIntDeinterlacer );

	deinterlacingGroup->insert( MedianDeinterlacer );

	deinterlacingGroup->insert( FfmpegDeinterlacer );


	deintSelectionWidgetLayout->addWidget( deinterlacingGroup, 0, 0 );

	PostprocessingOptions->insertTab( deintSelectionWidget, "" );

	tabLayout->addWidget( PostprocessingOptions/*, 1, 0*/ );

	PostprocessingOptions->setEnabled(false);
	connect( customPreset, SIGNAL (toggled(bool) ), customFilters, SLOT(setEnabled(bool)));
	connect( postProcessing, SIGNAL( toggled(bool) ), PostprocessingOptions, SLOT( setEnabled(bool) ) );
	connect( HzDeblockFilter, SIGNAL( toggled(bool) ), HzDeblockAQuality, SLOT( setEnabled(bool) ) );
	connect( HzDeblockFilter, SIGNAL( toggled(bool) ), HzDeblockCFiltering, SLOT( setEnabled(bool) ) );
	connect( VtDeblockFilter, SIGNAL( toggled(bool) ), VtDeblockCFiltering, SLOT( setEnabled(bool) ) );
	connect( VtDeblockFilter, SIGNAL( toggled(bool) ), VtDeblockAQuality, SLOT( setEnabled(bool) ) );
	connect( DeringFilter, SIGNAL( toggled(bool) ), DeringAQuality, SLOT( setEnabled(bool) ) );
	connect( DeringFilter, SIGNAL( toggled(bool) ), DeringCFiltering, SLOT( setEnabled(bool) ) );
	//connect( TmpNoiseFilter, SIGNAL( toggled(bool) ), TmpNoiseSlider, SLOT( setEnabled(bool) ) );

	connect( AutolevelsFilter, SIGNAL( toggled(bool) ), AutolevelsFullrange, SLOT( setEnabled(bool) ) );

	postProcessing->setText( i18n( "Enable use of postprocessing filters" ) );
	disablePPauto->setText( i18n( "Disable use of postprocessing when watching TV/DVD" ) );
	defaultPreset->setText( i18n( "Default" ) );
	QToolTip::add( defaultPreset, i18n( "Enable mplayer's default postprocessing filters" ) );
	customPreset->setText( i18n( "Custom" ) );
	QToolTip::add( customPreset, i18n( "Enable custom postprocessing filters (See: Custom preset -tab)" ) );
	fastPreset->setText( i18n( "Fast" ) );
	QToolTip::add( fastPreset, i18n( "Enable mplayer's fast postprocessing filters" ) );
	PostprocessingOptions->changeTab( presetSelectionWidget, i18n( "General" ) );
	customFilters->setTitle( QString::null );
	HzDeblockFilter->setText( i18n( "Horizontal deblocking" ) );
	VtDeblockFilter->setText( i18n( "Vertical deblocking" ) );
	DeringFilter->setText( i18n( "Dering filter" ) );
	HzDeblockAQuality->setText( i18n( "Auto quality" ) );
	QToolTip::add( HzDeblockAQuality, i18n( "Filter is used if there's enough CPU" ) );
	VtDeblockAQuality->setText( i18n( "Auto quality" ) );
	QToolTip::add( VtDeblockAQuality, i18n( "Filter is used if there's enough CPU" ) );
	DeringAQuality->setText( i18n( "Auto quality" ) );
	QToolTip::add( DeringAQuality, i18n( "Filter is used if there's enough CPU" ) );
	//QToolTip::add( TmpNoiseSlider, i18n( "Strength of the noise reducer" ) );
	AutolevelsFilter->setText( i18n( "Auto brightness/contrast" ) );
	AutolevelsFullrange->setText( i18n( "Stretch luminance to full range" ) );
	QToolTip::add( AutolevelsFullrange, i18n( "Stretches luminance to full range (0..255)" ) );
	HzDeblockCFiltering->setText( i18n( "Chrominance filtering" ) );
	VtDeblockCFiltering->setText( i18n( "Chrominance filtering" ) );
	DeringCFiltering->setText( i18n( "Chrominance filtering" ) );
	TmpNoiseFilter->setText( i18n( "Temporal noise reducer:" ) );
	PostprocessingOptions->changeTab( customFiltersWidget, i18n( "Custom Preset" ) );
	deinterlacingGroup->setTitle( QString::null );
	LinBlendDeinterlacer->setText( i18n( "Linear blend deinterlacer" ) );
	CubicIntDeinterlacer->setText( i18n( "Cubic interpolating deinterlacer" ) );
	LinIntDeinterlacer->setText( i18n( "Linear interpolating deinterlacer" ) );
	MedianDeinterlacer->setText( i18n( "Median deinterlacer" ) );
	FfmpegDeinterlacer->setText( i18n( "FFmpeg deinterlacer" ) );
	PostprocessingOptions->changeTab( deintSelectionWidget, i18n( "Deinterlacing" ) );
	PostprocessingOptions->adjustSize();
}

void KMPlayerPreferences::confirmDefaults() {
	switch( QMessageBox::warning( this, "KMPlayer",
        i18n("You are about to have all your settings overwritten with defaults.\nPlease confirm.\n"),
        i18n("Ok"), i18n("Cancel"), QString::null, 0, 1 ) ){
    		case 0:	KMPlayerPreferences::setDefaults();
        		break;
    		case 1:	break;
	}

}

void KMPlayerPreferences::setDefaults() {
	m_GeneralPageGeneral->keepSizeRatio->setChecked(true);
	m_GeneralPageGeneral->showConsoleOutput->setChecked(false);
	m_GeneralPageGeneral->loop->setChecked(false);
	m_GeneralPageGeneral->showControlButtons->setChecked(true);
	m_GeneralPageGeneral->autoHideControlButtons->setChecked(false);
	m_GeneralPageGeneral->showPositionSlider->setChecked(true);
	m_GeneralPageGeneral->seekTime->setValue(10);

	m_GeneralPageOutput->videoDriver->setCurrentItem (0);
	m_GeneralPageOutput->audioDriver->setCurrentItem(0);

	m_OPPagePostproc->postProcessing->setChecked(false);
	m_OPPagePostproc->disablePPauto->setChecked(true);

	m_OPPagePostproc->defaultPreset->setChecked(true);

	m_OPPagePostproc->LinBlendDeinterlacer->setChecked(false);
	m_OPPagePostproc->LinIntDeinterlacer->setChecked(false);
	m_OPPagePostproc->CubicIntDeinterlacer->setChecked(false);
	m_OPPagePostproc->MedianDeinterlacer->setChecked(false);
	m_OPPagePostproc->FfmpegDeinterlacer->setChecked(false);

}
#include "pref.moc"
