#ifndef carbonmenubarengine_h
#define carbonmenubarengine_h

//////////////////////////////////////////////////////////////////////////////
// carbonmenubarengine.h
// stores event filters and maps widgets to timelines for animations
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

#include "carbonbaseengine.h"
#include "carbondatamap.h"
#include "carbonmenubardata.h"

namespace Carbon
{

    //! stores menubar hovered action and timeLine
    class MenuBarBaseEngine: public BaseEngine
    {

        Q_OBJECT

        public:

        //! constructor
        explicit MenuBarBaseEngine( QObject* parent ):
            BaseEngine( parent )
        {}

        //! destructor
        virtual ~MenuBarBaseEngine( void )
        {}

        //! register menubar
        virtual bool registerWidget( QWidget* ) = 0;

        //! true if widget is animated
        virtual bool isAnimated( const QObject*, const QPoint& )
        { return false; }

        //! animation opacity
        virtual qreal opacity( const QObject*, const QPoint& )
        { return -1; }

        //! return 'hover' rect position when widget is animated
        virtual QRect currentRect( const QObject*, const QPoint& )
        { return QRect(); }

        //! animated rect
        virtual QRect animatedRect( const QObject* )
        { return QRect(); }

        //! timer
        virtual bool isTimerActive( const QObject* )
        { return false; }

        //! enability
        virtual void setEnabled( bool ) = 0;

        //! duration
        virtual void setDuration( int ) = 0;

        //! duration
        virtual void setFollowMouseDuration( int )
        {}

    };

    //! fading menubar animation
    class MenuBarEngineV1: public MenuBarBaseEngine
    {

        Q_OBJECT

        public:

        //! constructor
        explicit MenuBarEngineV1( QObject* parent ):
            MenuBarBaseEngine( parent )
        {}

        //! constructor
        MenuBarEngineV1( QObject* parent, MenuBarBaseEngine* other );

        //! destructor
        virtual ~MenuBarEngineV1( void )
        {}

        //! register menubar
        virtual bool registerWidget( QWidget* );

        //! true if widget is animated
        virtual bool isAnimated( const QObject* object, const QPoint& point );

        //! animation opacity
        virtual qreal opacity( const QObject* object, const QPoint& point )
        { return isAnimated( object, point ) ? _data.find( object ).data()->opacity( point ): AnimationData::OpacityInvalid; }

        //! return 'hover' rect position when widget is animated
        virtual QRect currentRect( const QObject* object, const QPoint& point)
        { return isAnimated( object, point ) ? _data.find( object ).data()->currentRect( point ): QRect(); }

        //! enability
        virtual void setEnabled( bool value )
        {
            BaseEngine::setEnabled( value );
            _data.setEnabled( value );
        }

        //! duration
        virtual void setDuration( int duration )
        {
            BaseEngine::setDuration( duration );
            _data.setDuration( duration );
        }

        //! return list of registered widgets
        virtual WidgetList registeredWidgets( void ) const;

        public Q_SLOTS:

        //! remove widget from map
        virtual bool unregisterWidget( QObject* object )
        { return _data.unregisterWidget( object ); }

        private:

        //! data map
        DataMap<MenuBarDataV1> _data;

    };

        //! follow-mouse menubar animation
    class MenuBarEngineV2: public MenuBarBaseEngine
    {

        Q_OBJECT

        public:

        //! constructor
        explicit MenuBarEngineV2( QObject* parent ):
            MenuBarBaseEngine( parent )
        {}

        //! constructor
        MenuBarEngineV2( QObject* parent, MenuBarBaseEngine* other );

        //! destructor
        virtual ~MenuBarEngineV2( void )
        {}

        //! register menubar
        virtual bool registerWidget( QWidget* );


        //! true if widget is animated
        virtual bool isAnimated( const QObject* object, const QPoint& point );

        //! animation opacity
        virtual qreal opacity( const QObject* object, const QPoint& point )
        { return isAnimated( object, point ) ? _data.find( object ).data()->opacity(): AnimationData::OpacityInvalid; }

        //! return 'hover' rect position when widget is animated
        virtual QRect currentRect( const QObject*, const QPoint& );

        //! return 'hover' rect position when widget is animated
        virtual QRect animatedRect( const QObject* );

        //! timer associated to the data
        virtual bool isTimerActive( const QObject* );

        //! enability
        virtual void setEnabled( bool value )
        {
            BaseEngine::setEnabled( value );
            _data.setEnabled( value );
        }

        //! duration
        virtual void setDuration( int value )
        {
            BaseEngine::setDuration( value );
            _data.setDuration( value );
        }

        //! duration
        virtual int followMouseDuration( void ) const
        { return _followMouseDuration; }

        //! duration
        virtual void setFollowMouseDuration( int duration )
        {
            _followMouseDuration = duration;
            foreach( const DataMap<MenuBarDataV2>::Value& value, _data )
            { if( value ) value.data()->setFollowMouseDuration( duration ); }
        }

        //! return list of registered widgets
        virtual WidgetList registeredWidgets( void ) const;

        protected Q_SLOTS:

        //! remove widget from map
        virtual bool unregisterWidget( QObject* object )
        { return _data.unregisterWidget( object ); }

        private:

        //! follow mouse animation duration
        int _followMouseDuration;

        //! data map
        DataMap<MenuBarDataV2> _data;

    };

}

#endif
