/* This file is part of the KMPlayer application
   Copyright (C) 2004 Koos Vriezen <koos.vriezen@xs4all.nl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qlayout.h>
#include <qlabel.h>
#include <qmap.h>
#include <qtimer.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qtable.h>
#include <qstringlist.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qgroupbox.h>
#include <qwhatsthis.h>
#include <qtabwidget.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qmessagebox.h>
#include <qpopupmenu.h>
#include <qsocket.h>

#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <klineedit.h>
#include <kurlrequester.h>
#include <kcombobox.h>
#include <kprocess.h>
#include <kconfig.h>
#include <kaction.h>
#include <kiconloader.h>

#include "kmplayer_backend_stub.h"
#include "kmplayer_callback.h"
#include "kmplayerpartbase.h"
#include "kmplayerconfig.h"
#include "kmplayervdr.h"
#include "kmplayer.h"

static const char * strVDR = "VDR";
static const char * strVDRPort = "Port";
static const char * strXVPort = "XV Port";
static const char * strXVScale = "XV Scale";

KDE_NO_CDTOR_EXPORT KMPlayerPrefSourcePageVDR::KMPlayerPrefSourcePageVDR (QWidget * parent)
 : QFrame (parent) {
    //KURLRequester * v4ldevice;
    QVBoxLayout *layout = new QVBoxLayout (this, 5, 2);
    QGridLayout *gridlayout = new QGridLayout (layout, 2, 2);
    QLabel * label = new QLabel (i18n ("XVideo port:"), this);
    gridlayout->addWidget (label, 0, 0);
    xv_port = new QLineEdit ("", this);
    QWhatsThis::add (xv_port, i18n ("Port base of the X Video extension.\nIf left to default (0), the first available port will be used. However if you have multiple XVideo instances, you might have to provide the port to use here.\nSee the output from 'xvinfo' for more information"));
    gridlayout->addWidget (xv_port, 0, 1);
    label = new QLabel (i18n ("Communication port:"), this);
    gridlayout->addWidget (label, 1, 0);
    tcp_port = new QLineEdit ("", this);
    QWhatsThis::add (tcp_port, i18n ("Communication port with VDR. Default is port 2001.\nIf you use another port, with the '-p' option of 'vdr', you must set it here too."));
    gridlayout->addWidget (tcp_port, 1, 1);
    layout->addLayout (gridlayout);
    scale = new QButtonGroup (2, Qt::Vertical, i18n ("Scale"), this);
    new QRadioButton (i18n ("4:3"), scale);
    new QRadioButton (i18n ("16:9"), scale);
    QWhatsThis::add (scale, i18n ("Aspects to use when viewing VDR"));
    scale->setButton (0);
    layout->addWidget (scale);
    layout->addItem (new QSpacerItem (5, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

KDE_NO_CDTOR_EXPORT KMPlayerPrefSourcePageVDR::~KMPlayerPrefSourcePageVDR () {}

//-----------------------------------------------------------------------------

static const char * cmd_chan_query = "CHAN\n";
static const char * cmd_list_channels = "LSTC\n";

class VDRCommand {
public:
    KDE_NO_CDTOR_EXPORT VDRCommand (const char * c, VDRCommand * n=0L)
        : command (c), next (n) {}
    KDE_NO_CDTOR_EXPORT ~VDRCommand () {}
    const char * command;
    VDRCommand * next;
};

//-----------------------------------------------------------------------------

KDE_NO_CDTOR_EXPORT KMPlayerVDRSource::KMPlayerVDRSource (KMPlayerApp * app, QPopupMenu * m)
 : KMPlayerMenuSource (i18n ("VDR"), app, m, "vdrsource"),
   m_configpage (0),
   m_socket (new QSocket (this)), 
   commands (0L),
   channel_timer (0),
   timeout_timer (0),
   tcp_port (0),
   xv_port (0) {
    m_player->settings ()->addPage (this);
    act_up = new KAction (i18n ("VDR Key Up"), 0, 0, this, SLOT (keyUp ()), m_app->actionCollection (),"vdr_key_up");
    act_down = new KAction (i18n ("VDR Key Down"), 0, 0, this, SLOT (keyDown ()), m_app->actionCollection (),"vdr_key_down");
    act_back = new KAction (i18n ("VDR Key Back"), 0, 0, this, SLOT (keyBack ()), m_app->actionCollection (),"vdr_key_back");
    act_ok = new KAction (i18n ("VDR Key Ok"), 0, 0, this, SLOT (keyOk ()), m_app->actionCollection (),"vdr_key_ok");
    act_setup = new KAction (i18n ("VDR Key Setup"), 0, 0, this, SLOT (keySetup ()), m_app->actionCollection (),"vdr_key_setup");
    act_channels = new KAction (i18n ("VDR Key Channels"), 0, 0, this, SLOT (keyChannels ()), m_app->actionCollection (),"vdr_key_channels");
    act_menu = new KAction (i18n ("VDR Key Menu"), 0, 0, this, SLOT (keyMenu ()), m_app->actionCollection (),"vdr_key_menu");
    act_red = new KAction (i18n ("VDR Key Red"), 0, 0, this, SLOT (keyRed ()), m_app->actionCollection (),"vdr_key_red");
    act_green = new KAction (i18n ("VDR Key Green"), 0, 0, this, SLOT (keyGreen ()), m_app->actionCollection (),"vdr_key_green");
    act_yellow = new KAction (i18n ("VDR Key Yellow"), 0, 0, this, SLOT (keyYellow ()), m_app->actionCollection (),"vdr_key_yellow");
    act_blue = new KAction (i18n ("VDR Key Blue"), 0, 0, this, SLOT (keyBlue ()), m_app->actionCollection (),"vdr_key_blue");
}

KDE_NO_CDTOR_EXPORT KMPlayerVDRSource::~KMPlayerVDRSource () {
}

KDE_NO_EXPORT bool KMPlayerVDRSource::hasLength () {
    return false;
}

KDE_NO_EXPORT bool KMPlayerVDRSource::isSeekable () {
    return true;
}

KDE_NO_EXPORT QString KMPlayerVDRSource::prettyName () {
    return i18n ("VDR");
}

KDE_NO_EXPORT void KMPlayerVDRSource::activate () {
    KMPlayerView * view = static_cast <KMPlayerView *> (m_player->view ());
    view->addFullscreenAction (i18n ("VDR Key Up"), act_up->shortcut (), this, SLOT (keyUp ()), "vdr_key_up");
    view->addFullscreenAction (i18n ("VDR Key Down"), act_down->shortcut (), this, SLOT (keyDown ()), "vdr_key_down");
    view->addFullscreenAction (i18n ("VDR Key Ok"), act_ok->shortcut (), this, SLOT (keyOk ()), "vdr_key_ok");
    view->addFullscreenAction (i18n ("VDR Key Back"), act_back->shortcut (), this, SLOT (keyBack ()), "vdr_key_back");
    view->addFullscreenAction (i18n ("VDR Key Setup"), act_setup->shortcut (), this, SLOT (keySetup ()), "vdr_key_setup");
    view->addFullscreenAction (i18n ("VDR Key Channels"), act_channels->shortcut (), this, SLOT (keyChannels ()), "vdr_key_channels");
    view->addFullscreenAction (i18n ("VDR Key Menu"), act_menu->shortcut (), this, SLOT (keyMenu ()), "vdr_key_menu");
    view->addFullscreenAction (i18n ("VDR Key 0"), KShortcut (Qt::Key_0), this, SLOT (key0 ()), "vdr_key_0");
    view->addFullscreenAction (i18n ("VDR Key 1"), KShortcut (Qt::Key_1), this, SLOT (key1 ()), "vdr_key_1");
    view->addFullscreenAction (i18n ("VDR Key 2"), KShortcut (Qt::Key_2), this, SLOT (key2 ()), "vdr_key_2");
    view->addFullscreenAction (i18n ("VDR Key 3"), KShortcut (Qt::Key_3), this, SLOT (key3 ()), "vdr_key_3");
    view->addFullscreenAction (i18n ("VDR Key 4"), KShortcut (Qt::Key_4), this, SLOT (key4 ()), "vdr_key_4");
    view->addFullscreenAction (i18n ("VDR Key 5"), KShortcut (Qt::Key_5), this, SLOT (key5 ()), "vdr_key_5");
    view->addFullscreenAction (i18n ("VDR Key 6"), KShortcut (Qt::Key_6), this, SLOT (key6 ()), "vdr_key_6");
    view->addFullscreenAction (i18n ("VDR Key 7"), KShortcut (Qt::Key_7), this, SLOT (key7 ()), "vdr_key_7");
    view->addFullscreenAction (i18n ("VDR Key 8"), KShortcut (Qt::Key_8), this, SLOT (key8 ()), "vdr_key_8");
    view->addFullscreenAction (i18n ("VDR Key 9"), KShortcut (Qt::Key_9), this, SLOT (key9 ()), "vdr_key_9");
    view->addFullscreenAction (i18n ("VDR Key Red"), act_red->shortcut (), this, SLOT (keyRed ()), "vdr_key_red");
    view->addFullscreenAction (i18n ("VDR Key Green"), act_green->shortcut (), this, SLOT (keyGreen ()), "vdr_key_green");
    view->addFullscreenAction (i18n ("VDR Key Yellow"), act_yellow->shortcut (), this, SLOT (keyYellow ()), "vdr_key_yellow");
    view->addFullscreenAction (i18n ("VDR Key Blue"), act_blue->shortcut (), this, SLOT (keyBlue ()), "vdr_key_blue");
    m_player->setProcess ("xvideo");
    static_cast<XVideo *>(m_player->players () ["xvideo"])->setPort (xv_port);
    m_menu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("up"), KIcon::Small, 0, true), i18n ("Up"), this, SLOT (keyUp ()), 0, -1, 1);
    m_menu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("down"), KIcon::Small, 0, true), i18n ("Down"), this, SLOT (keyUp ()), 0, -1, 2);
    m_menu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("back"), KIcon::Small, 0, true), i18n ("Back"), this, SLOT (keyBack ()), 0, -1, 3);
    m_menu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("ok"), KIcon::Small, 0, true), i18n ("Ok"), this, SLOT (keyOk ()), 0, -1, 4);
    m_menu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("player_playlist"), KIcon::Small, 0, true), i18n ("Channels"), this, SLOT (keyChannels ()), 0, -1, 5);
    m_menu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("configure"), KIcon::Small, 0, true), i18n ("Setup"), this, SLOT (keySetup ()), 0, -1, 6);
    m_menu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("showmenu"), KIcon::Small, 0, true), i18n ("Menu"), this, SLOT (keyMenu ()), 0, -1, 7);
    m_menu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("red"), KIcon::Small, 0, true), i18n ("Red"), this, SLOT (keyRed ()), 0, -1, 8);
    m_menu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("green"), KIcon::Small, 0, true), i18n ("Green"), this, SLOT (keyGreen ()), 0, -1, 9);
    m_menu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("yellow"), KIcon::Small, 0, true), i18n ("Yellow"), this, SLOT (keyYellow ()), 0, -1, 10);
    m_menu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("blue"), KIcon::Small, 0, true), i18n ("Blue"), this, SLOT (keyBlue ()), 0, -1, 11);
    connect (m_socket, SIGNAL (connected ()), this, SLOT (connected ()));
    connect (m_socket, SIGNAL (readyRead ()), this, SLOT (readyRead ()));
    connect (m_socket, SIGNAL (connectionClosed ()), this, SLOT (disconnected ()));
    connect (m_socket, SIGNAL (error (int)), this, SLOT (socketError (int)));
    KMPlayerControlPanel * panel = m_app->view()->buttonBar ();
    panel->button (KMPlayerControlPanel::button_red)->show ();
    panel->button (KMPlayerControlPanel::button_green)->show ();
    panel->button (KMPlayerControlPanel::button_yellow)->show ();
    panel->button (KMPlayerControlPanel::button_blue)->show ();
    panel->button (KMPlayerControlPanel::button_pause)->hide ();
    panel->button (KMPlayerControlPanel::button_record)->hide ();
    connect (panel->button (KMPlayerControlPanel::button_red), SIGNAL (clicked ()), this, SLOT (keyRed ()));
    connect (panel->button (KMPlayerControlPanel::button_green), SIGNAL (clicked ()), this, SLOT (keyGreen ()));
    connect (panel->button (KMPlayerControlPanel::button_yellow), SIGNAL (clicked ()), this, SLOT (keyYellow ()));
    connect (panel->button (KMPlayerControlPanel::button_blue), SIGNAL (clicked ()), this, SLOT (keyBlue ()));
    m_document = (new Document (QString ("VDR")))->self ();
    queueCommand (cmd_list_channels);
    setAspect (scale ? 16.0/9 : 1.33);
    if (m_player->settings ()->sizeratio)
        view->viewer ()->setAspect (aspect ());
}

KDE_NO_EXPORT void KMPlayerVDRSource::deactivate () {
    disconnect (m_socket, SIGNAL (connected ()), this, SLOT (connected ()));
    disconnect (m_socket, SIGNAL (readyRead ()), this, SLOT (readyRead ()));
    disconnect (m_socket, SIGNAL (connectionClosed ()), this, SLOT (disconnected ()));
    disconnect (m_socket, SIGNAL (error (int)), this, SLOT (socketError (int)));
    if (m_player->view ()) {
        KMPlayerControlPanel * panel = m_app->view()->buttonBar ();
        disconnect (panel->button (KMPlayerControlPanel::button_red), SIGNAL (clicked ()), this, SLOT (keyRed ()));
        disconnect (panel->button (KMPlayerControlPanel::button_green), SIGNAL (clicked ()), this, SLOT (keyGreen ()));
        disconnect (panel->button (KMPlayerControlPanel::button_yellow), SIGNAL (clicked ()), this, SLOT (keyYellow ()));
        disconnect (panel->button (KMPlayerControlPanel::button_blue), SIGNAL (clicked ()), this, SLOT (keyBlue ()));
    }
    if (m_socket->state () == QSocket::Connected)
        queueCommand ("QUIT\n");
    m_menu->removeItemAt (11);
    m_menu->removeItemAt (10);
    m_menu->removeItemAt (9);
    m_menu->removeItemAt (8);
    m_menu->removeItemAt (7);
    m_menu->removeItemAt (6);
    m_menu->removeItemAt (5);
    m_menu->removeItemAt (4);
    m_menu->removeItemAt (3);
    m_menu->removeItemAt (2);
    m_menu->removeItemAt (1);
    deleteCommands ();
    killTimer (channel_timer);
    channel_timer = 0;
    if (m_document)
        m_document->document ()->dispose ();
    m_document = 0L;
}

KDE_NO_EXPORT void KMPlayerVDRSource::connected () {
    kdDebug() << "connected " << commands << endl;
    if (!m_player->players () ["xvideo"]->playing ()) {
        QTimer::singleShot (0, m_player, SLOT (play ()));
    }
    channel_timer = startTimer (3000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::disconnected () {
    killTimer (channel_timer);
    channel_timer = 0;
    deleteCommands ();
}

static struct ReadBuf {
    char * buf;
    int length;
    KDE_NO_CDTOR_EXPORT ReadBuf () : buf (0L), length (0) {}
    KDE_NO_CDTOR_EXPORT ~ReadBuf () {
        clear ();
    }
    KDE_NO_EXPORT void operator += (const char * s) {
        int l = strlen (s);
        char * b = new char [length + l + 1];
        if (length)
            strcpy (b, buf);
        strcpy (b + length, s);
        length += l;
        delete buf;
        buf = b;
    }
    KDE_NO_EXPORT QCString mid (int p) {
        return QCString (buf + p);
    }
    KDE_NO_EXPORT QCString left (int p) {
        return QCString (buf, p);
    }
    KDE_NO_EXPORT QCString getReadLine ();
    KDE_NO_EXPORT void clear () {
        delete [] buf;
        buf = 0;
        length = 0;
    }
} readbuf;

KDE_NO_EXPORT QCString ReadBuf::getReadLine () {
    QCString out;
    if (!length)
        return out;
    int p = strcspn (buf, "\r\n");
    if (p < length) {
        int skip = strspn (buf + p, "\r\n");
        out = left (p+1);
        int nl = length - p - skip;
        memmove (buf, buf + p + skip, nl + 1);
        length = nl;
    }
    return out;
}

KDE_NO_EXPORT void KMPlayerVDRSource::readyRead () {
    KMPlayerView * v = static_cast <KMPlayerView *> (m_player->view ());
    long nr = m_socket->bytesAvailable();
    char * data = new char [nr + 1];
    m_socket->readBlock (data, nr);
    if (!v)
        return;
    data [nr] = 0;
    readbuf += data;
    QCString line = readbuf.getReadLine ();
    if (commands) {
        bool cmd_done = false;
        while (!line.isEmpty ()) {
            cmd_done = (line.length () > 3 && line[3] == ' '); // from svdrpsend.pl
            if (!strcmp (commands->command, cmd_list_channels) && m_document) {
                int p = line.find (';');
                if (p > 0)
                    line.truncate (p);
                m_document->appendChild ((new GenericURL (m_document, line.mid (4)))->self ());
                if (cmd_done)
                    m_player->updateTree (m_document, m_current);
            } else if (!strcmp (commands->command, cmd_chan_query)) {
                if (line.length () > 4) {
                    m_player->changeTitle (line.mid (4));
                    v->playList ()->selectItem (line.mid (4));
                    if (cmd_done) {
                        killTimer (channel_timer);
                        channel_timer = startTimer (30000);
                    }
                }
            }
            line = readbuf.getReadLine ();
        }
        if (cmd_done) {
            VDRCommand * c = commands->next;
            delete commands;
            commands = c;
            if (commands)
                sendCommand ();
        }
    }
    delete [] data;
}

KDE_NO_EXPORT void KMPlayerVDRSource::socketError (int code) {
    if (code == QSocket::ErrHostNotFound) {
        KMessageBox::error (m_configpage, i18n ("Host not found"), i18n ("Error"));
    } else if (code == QSocket::ErrConnectionRefused) {
        KMessageBox::error (m_configpage, i18n ("Connection refused"), i18n ("Error"));
    }
}

KDE_NO_EXPORT void KMPlayerVDRSource::queueCommand (const char * cmd) {
    if (m_player->process ()->source () != this)
        return;
    if (!commands) {
        readbuf.clear ();
        commands = new VDRCommand (cmd);
        if (m_socket->state () == QSocket::Connected) {
            sendCommand ();
        } else {
            m_socket->connectToHost ("127.0.0.1", tcp_port);
            commands = new VDRCommand ("connect", commands);
        }
    } else {
        VDRCommand * c = commands;
        for (int i = 0; i < 10; ++i)
            if (!c->next) {
                c->next = new VDRCommand (cmd);
                break;
            }
    }
}

KDE_NO_EXPORT void KMPlayerVDRSource::queueCommand (const char * cmd, int t) {
    queueCommand (cmd);
    killTimer (channel_timer);
    channel_timer = startTimer (t);
}

KDE_NO_EXPORT void KMPlayerVDRSource::sendCommand () {
    m_socket->writeBlock (commands->command, strlen(commands->command));
    m_socket->flush ();
    killTimer (timeout_timer);
    timeout_timer = startTimer (30000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::timerEvent (QTimerEvent * e) {
    if (e->timerId () == timeout_timer) {
        deleteCommands ();
    } else
        queueCommand (cmd_chan_query);
}

KDE_NO_EXPORT void KMPlayerVDRSource::deleteCommands () {
    killTimer (timeout_timer);
    timeout_timer = 0;
    for (VDRCommand * c = commands; c; c = commands) {
        commands = commands->next;
        delete c;
    }
}

KDE_NO_EXPORT void KMPlayerVDRSource::jump (ElementPtr e) {
    if (!e->isMrl ()) return;
    QCString c ("CHAN ");
    QCString ch = e->mrl ()->src.local8Bit ();
    int p = ch.find (' ');
    if (p > 0)
        c += ch.left (p);
    else
        c += ch; // hope for the best ..
    c += "\n";
    queueCommand (c);
}

KDE_NO_EXPORT void KMPlayerVDRSource::forward () {
    queueCommand ("CHAN +\n");
}

KDE_NO_EXPORT void KMPlayerVDRSource::backward () {
    queueCommand ("CHAN -\n");
}

KDE_NO_EXPORT void KMPlayerVDRSource::keyUp () {
    queueCommand ("HITK UP\n", 1000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::keyDown () {
    queueCommand ("HITK DOWN\n", 1000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::keyBack () {
    queueCommand ("HITK BACK\n");
}

KDE_NO_EXPORT void KMPlayerVDRSource::keyOk () {
    queueCommand ("HITK OK\n");
}

KDE_NO_EXPORT void KMPlayerVDRSource::keySetup () {
    queueCommand ("HITK SETUP\n");
}

KDE_NO_EXPORT void KMPlayerVDRSource::keyChannels () {
    queueCommand ("HITK CHANNELS\n");
}

KDE_NO_EXPORT void KMPlayerVDRSource::keyMenu () {
    queueCommand ("HITK MENU\n");
}

KDE_NO_EXPORT void KMPlayerVDRSource::key0 () {
    queueCommand ("HITK 0\n", 2000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::key1 () {
    queueCommand ("HITK 1\n", 2000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::key2 () {
    queueCommand ("HITK 2\n", 2000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::key3 () {
    queueCommand ("HITK 3\n", 2000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::key4 () {
    queueCommand ("HITK 4\n", 2000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::key5 () {
    queueCommand ("HITK 5\n", 2000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::key6 () {
    queueCommand ("HITK 6\n", 2000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::key7 () {
    queueCommand ("HITK 7\n", 2000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::key8 () {
    queueCommand ("HITK 8\n", 2000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::key9 () {
    queueCommand ("HITK 9\n", 2000);
}

KDE_NO_EXPORT void KMPlayerVDRSource::keyRed () {
    queueCommand ("HITK RED\n");
}

KDE_NO_EXPORT void KMPlayerVDRSource::keyGreen () {
    queueCommand ("HITK GREEN\n");
}

KDE_NO_EXPORT void KMPlayerVDRSource::keyYellow () {
    queueCommand ("HITK YELLOW\n");
}

KDE_NO_EXPORT void KMPlayerVDRSource::keyBlue () {
    queueCommand ("HITK BLUE\n");
}

KDE_NO_EXPORT void KMPlayerVDRSource::write (KConfig * m_config) {
    m_config->setGroup (strVDR);
    m_config->writeEntry (strVDRPort, tcp_port);
    m_config->writeEntry (strXVPort, xv_port);
    m_config->writeEntry (strXVScale, scale);
}

KDE_NO_EXPORT void KMPlayerVDRSource::read (KConfig * m_config) {
    m_config->setGroup (strVDR);
    tcp_port = m_config->readNumEntry (strVDRPort, 2001);
    xv_port = m_config->readNumEntry (strXVPort, 0);
    scale = m_config->readNumEntry (strXVScale, 0);
}

KDE_NO_EXPORT void KMPlayerVDRSource::sync (bool fromUI) {
    if (fromUI) {
        tcp_port = m_configpage->tcp_port->text ().toInt ();
        xv_port = m_configpage->xv_port->text ().toInt ();
        scale = m_configpage->scale->id (m_configpage->scale->selected ());
        setAspect (scale ? 16.0/9 : 1.25);
        static_cast<XVideo *>(m_player->players()["xvideo"])->setPort(xv_port);
    } else {
        m_configpage->tcp_port->setText (QString::number (tcp_port));
        m_configpage->xv_port->setText (QString::number (xv_port));
        m_configpage->scale->setButton (scale);
    }
}

KDE_NO_EXPORT void KMPlayerVDRSource::prefLocation (QString & item, QString & icon, QString & tab) {
    item = i18n ("Source");
    icon = QString ("source");
    tab = i18n ("VDR");
}

KDE_NO_EXPORT QFrame * KMPlayerVDRSource::prefPage (QWidget * parent) {
    if (!m_configpage)
        m_configpage = new KMPlayerPrefSourcePageVDR (parent);
    return m_configpage;
}
//-----------------------------------------------------------------------------

KDE_NO_CDTOR_EXPORT XVideo::XVideo (KMPlayer * player)
 : KMPlayerCallbackProcess (player, "xvideo"), xv_port (0) {}

KDE_NO_CDTOR_EXPORT XVideo::~XVideo () {}

KDE_NO_EXPORT void XVideo::initProcess () {
    KMPlayerProcess::initProcess ();
    connect (m_process, SIGNAL (processExited (KProcess *)),
            this, SLOT (processStopped (KProcess *)));
    connect (m_process, SIGNAL (receivedStdout (KProcess *, char *, int)),
            this, SLOT (processOutput (KProcess *, char *, int)));
    connect (m_process, SIGNAL (receivedStderr (KProcess *, char *, int)),
            this, SLOT (processOutput (KProcess *, char *, int)));
}

KDE_NO_EXPORT bool XVideo::play () {
    if (playing ()) {
        return true;
    }
    initProcess ();
    QString cmd  = QString ("kxvplayer -port %1 -wid %2 -cb %3 ").arg (xv_port).arg (view()->viewer()->embeddedWinId ()).arg (dcopName ());
    printf ("%s\n", cmd.latin1 ());
    *m_process << cmd;
    m_process->start (KProcess::NotifyOnExit, KProcess::All);
    if (m_process->isRunning ())
        emit startedPlaying ();
    return m_process->isRunning ();
}

KDE_NO_EXPORT bool XVideo::quit () {
    if (!playing ()) return true;
    if (m_backend)
        m_backend->quit ();
    else if (view ())
        view ()->viewer ()->sendKeyEvent ('q');
#if KDE_IS_VERSION(3, 1, 90)
    m_process->wait(2);
#else
    QTime t;
    t.start ();
    do {
        KProcessController::theKProcessController->waitForProcessExit (2);
    } while (t.elapsed () < 2000 && m_process->isRunning ());
#endif
    return KMPlayerProcess::stop ();
}

KDE_NO_EXPORT void XVideo::setStarted (QByteArray & data) {
    QString dcopname;
    dcopname.sprintf ("kxvideoplayer-%u", m_process->pid ());
    kdDebug () << "up and running " << dcopname << endl;
    m_backend = new KMPlayerBackend_stub (dcopname.ascii (), "KMPlayerBackend");
    KMPlayerCallbackProcess::setStarted (data);
    m_backend->play ();
}

KDE_NO_EXPORT void XVideo::processStopped (KProcess *) {
    QTimer::singleShot (0, this, SLOT (emitFinished ()));
}

KDE_NO_EXPORT void XVideo::processOutput (KProcess *, char * str, int slen) {
    KMPlayerView * v = view ();
    if (v && slen > 0)
        v->addText (QString::fromLocal8Bit (str, slen));
}

#include "kmplayervdr.moc"
