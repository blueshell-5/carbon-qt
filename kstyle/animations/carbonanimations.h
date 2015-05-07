#ifndef carbonanimations_h
#define carbonanimations_h

//////////////////////////////////////////////////////////////////////////////
// carbonanimations.h
// container for all animation engines
// -------------------
//
// Copyright (c) 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#include "carbonbusyindicatorengine.h"
#include "carbondockseparatorengine.h"
#include "carbonheaderviewengine.h"
#include "carbonmdiwindowengine.h"
#include "carbonmenubarengine.h"
#include "carbonmenuengine.h"
#include "carbonprogressbarengine.h"
#include "carbonscrollbarengine.h"
#include "carbonspinboxengine.h"
#include "carbonsplitterengine.h"
#include "carbontabbarengine.h"
#include "carbontoolbarengine.h"
#include "carbontoolboxengine.h"
#include "carbonwidgetstateengine.h"

#include <QObject>
#include <QList>

namespace Carbon
{

    //! stores engines
    class Animations: public QObject
    {

        Q_OBJECT

        public:

        //! constructor
        explicit Animations( QObject* );

        //! destructor
        virtual ~Animations( void )
        {}

        //! register animations corresponding to given widget, depending on its type.
        void registerWidget( QWidget* widget ) const;

        /*! unregister all animations associated to a widget */
        void unregisterWidget( QWidget* widget ) const;

        //! enability engine
        WidgetStateEngine& widgetEnabilityEngine( void ) const
        { return *_widgetEnabilityEngine; }

        //! abstractButton engine
        WidgetStateEngine& widgetStateEngine( void ) const
        { return *_widgetStateEngine; }

        //! editable combobox arrow hover engine
        WidgetStateEngine& comboBoxEngine( void ) const
        { return *_comboBoxEngine; }

        //! Tool buttons arrow hover engine
        WidgetStateEngine& toolButtonEngine( void ) const
        { return *_toolButtonEngine; }

        //! item view engine
        WidgetStateEngine& inputWidgetEngine( void ) const
        { return *_inputWidgetEngine; }

        //! splitter engine
        SplitterEngine& splitterEngine( void ) const
        { return *_splitterEngine; }

        //! busy indicator
        BusyIndicatorEngine& busyIndicatorEngine( void ) const
        { return *_busyIndicatorEngine; }

        //! dock separators engine
        DockSeparatorEngine& dockSeparatorEngine( void ) const
        { return *_dockSeparatorEngine; }

        //! header view engine
        HeaderViewEngine& headerViewEngine( void ) const
        { return *_headerViewEngine; }

        //! progressbar engine
        ProgressBarEngine& progressBarEngine( void ) const
        { return *_progressBarEngine; }

        //! menubar engine
        MenuBarBaseEngine& menuBarEngine( void ) const
        { return *_menuBarEngine; }

        //! menu engine
        MenuBaseEngine& menuEngine( void ) const
        { return *_menuEngine; }

        //! scrollbar engine
        ScrollBarEngine& scrollBarEngine( void ) const
        { return *_scrollBarEngine; }

        //! spinbox engine
        SpinBoxEngine& spinBoxEngine( void ) const
        { return *_spinBoxEngine; }

        //! tabbar
        TabBarEngine& tabBarEngine( void ) const
        { return *_tabBarEngine; }

        //! toolbar
        ToolBarEngine& toolBarEngine( void ) const
        { return *_toolBarEngine; }

        //! toolbox
        ToolBoxEngine& toolBoxEngine( void ) const
        { return *_toolBoxEngine; }

        //! mdi windows
        MdiWindowEngine& mdiWindowEngine( void ) const
        { return *_mdiWindowEngine; }

        //! setup engines
        void setupEngines( void );

        protected Q_SLOTS:

        //! enregister engine
        void unregisterEngine( QObject* );
        private:

        //! register new engine
        void registerEngine( BaseEngine* engine );

        //! busy indicator
        BusyIndicatorEngine* _busyIndicatorEngine;

        //! dock separator handle hover effect
        DockSeparatorEngine* _dockSeparatorEngine;

        //! headerview hover effect
        HeaderViewEngine* _headerViewEngine;

        //! widget enability engine
        WidgetStateEngine* _widgetEnabilityEngine;

        //! abstract button engine
        WidgetStateEngine* _widgetStateEngine;

        //! editable combobox arrow hover effect
        WidgetStateEngine* _comboBoxEngine;

        //! mennu toolbutton arrow hover effect
        WidgetStateEngine* _toolButtonEngine;

        //! item view engine
        WidgetStateEngine* _inputWidgetEngine;

        //! QSplitter engine
        SplitterEngine* _splitterEngine;

        //! progressbar engine
        ProgressBarEngine* _progressBarEngine;

        //! menubar engine
        MenuBarBaseEngine* _menuBarEngine;

        //! menu engine
        MenuBaseEngine* _menuEngine;

        //! scrollbar engine
        ScrollBarEngine* _scrollBarEngine;

        //! spinbox engine
        SpinBoxEngine* _spinBoxEngine;

        //! tabbar engine
        TabBarEngine* _tabBarEngine;

        //! toolbar engine
        ToolBarEngine* _toolBarEngine;

        //! toolbar engine
        ToolBoxEngine* _toolBoxEngine;

        //! mdi window
        MdiWindowEngine* _mdiWindowEngine;

        //! keep list of existing engines
        QList< BaseEngine::Pointer > _engines;

    };

}

#endif