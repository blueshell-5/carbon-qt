/*
 * Copyright 2009-2010 Hugo Pereira Da Costa <hugo.pereira@free.fr>
 * Copyright 2008 Long Huynh Huu <long.upcase@googlemail.com>
 * Copyright 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
 * Copyright 2007 Casper Boemann <cbr@boemann.dk>
 * Copyright 2007 Fredrik H?glund <fredrik@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "carbonhelper.h"
#include "libcarbon.h"

#include <KColorUtils>
#include <KColorScheme>

#if CARBON_USE_KDE4
#include <KGlobalSettings>
#endif

#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QTextStream>
#include <math.h>

#if CARBON_HAVE_X11
#include <QX11Info>
#endif

#if CARBON_HAVE_X11 && QT_VERSION < 0x050000
#include <X11/Xlib-xcb.h>
#endif

namespace Carbon
{

    const qreal Helper::_slabThickness = 0.45;
    const qreal Helper::_shadowGain = 1.5;
    const qreal Helper::_glowBias = 0.6;

    //____________________________________________________________________
    Helper::Helper( KSharedConfig::Ptr config ):
        _config( config )
    { init(); }

    //____________________________________________________________________
    #if CARBON_USE_KDE4
    Helper::Helper( const QByteArray& name ):
        _componentData( name, 0, KComponentData::SkipMainComponentRegistration ),
        _config( _componentData.config() )
    { init(); }
    #endif

    //____________________________________________________________________
    KSharedConfig::Ptr Helper::config() const
    { return _config; }

    //____________________________________________________________________
    void Helper::loadConfig()
    {

        #if CARBON_USE_KDE4
        _contrast = KGlobalSettings::contrastF( _config );
        #else
        _contrast = KColorScheme::contrastF( _config );
        #endif

        _bgcontrast = qMin( 1.0, 0.9*_contrast/0.7 );

        _viewFocusBrush = KStatefulBrush( KColorScheme::View, KColorScheme::FocusColor, _config );
        _viewHoverBrush = KStatefulBrush( KColorScheme::View, KColorScheme::HoverColor, _config );
        _viewNegativeTextBrush = KStatefulBrush( KColorScheme::View, KColorScheme::NegativeText, _config );

    }

        //____________________________________________________________________
    void Helper::invalidateCaches()
    {
        _slabCache.clear();
        _slabSunkenCache.clear();
        _decoColorCache.clear();
        _lightColorCache.clear();
        _darkColorCache.clear();
        _shadowColorCache.clear();
        _backgroundTopColorCache.clear();
        _backgroundBottomColorCache.clear();
        _backgroundRadialColorCache.clear();
        _backgroundColorCache.clear();
        _backgroundCache.clear();
        _dotCache.clear();
    }

    //____________________________________________________________________
    void Helper::setMaxCacheSize( int value )
    {

        // assign value
        _slabCache.setMaxCacheSize( value );
        _slabSunkenCache.setMaxCost( value );
        _backgroundCache.setMaxCost( value );
        _dotCache.setMaxCost( value );

        /* note: we do not limit the size of the color caches on purpose, since they should be small anyway */

    }

    //____________________________________________________________________
    void Helper::renderWindowBackground( QPainter* p, const QRect& clipRect, const QWidget* widget, const QWidget* window, const QColor& color, int yShift, int gradientHeight )
    {

        // get coordinates relative to the client area
        // this is stupid. One could use mapTo if this was taking const QWidget* and not
        // QWidget* as argument.
        const QWidget* w( widget );
        int x( 0 );
        int y( -yShift );

        while ( w != window && !w->isWindow() && w != w->parentWidget() )
        {
            x += w->geometry().x();
            y += w->geometry().y();
            w = w->parentWidget();
        }

        if ( clipRect.isValid() )
        {
            p->save();
            p->setClipRegion( clipRect,Qt::IntersectClip );
        }

        // calculate upper part height
        // special tricks are needed
        // to handle both window contents and window decoration
        const QRect r = window->rect();
        int height( window->frameGeometry().height() );
        int width( window->frameGeometry().width() );

        // gradient offset
        const int offset( gradientHeight - 20 );

        // draw upper linear gradient
        const int splitY( offset + qMin( 300, ( 3*height )/4 ) );
        const QRect upperRect( -x, -y, r.width(), splitY );
        QPixmap tile( verticalGradient( color, splitY, offset ) );
        p->drawTiledPixmap( upperRect, tile );

        // draw lower flat part
        const QRect lowerRect( -x, splitY-y, r.width(), r.height() - splitY-yShift );
        p->fillRect( lowerRect, backgroundBottomColor( color ) );

        // draw upper radial gradient
        const int radialW( qMin( 600, width ) );
        const QRect radialRect( ( r.width() - radialW ) / 2-x, -y, radialW, offset + 64 );
        if ( clipRect.intersects( radialRect ) )
        {
            tile = radialGradient( color, radialW, offset + 64 );
            p->drawPixmap( radialRect, tile );
        }

        if ( clipRect.isValid() )
        { p->restore(); }
    }


    //____________________________________________________________________
    void Helper::renderBackgroundPixmap( QPainter* p, const QRect& clipRect, const QWidget* widget, const QWidget* window, int yShift, int gradientHeight )
    {

        // background pixmap
        if( _backgroundPixmap.isNull() ) return;

        // get coordinates relative to the client area
        // this is stupid. One could use mapTo if this was taking const QWidget* and not
        // QWidget* as argument.
        const QWidget* w( widget );
        int x( 0 );
        int y( -yShift );

        while ( w != window && !w->isWindow() && w != w->parentWidget() )
        {
            x += w->geometry().x();
            y += w->geometry().y();
            w = w->parentWidget();
        }

        if ( clipRect.isValid() )
        {
            p->save();
            p->setClipRegion( clipRect,Qt::IntersectClip );
        }

        // calculate upper part height
        // special tricks are needed
        // to handle both window contents and window decoration
        int height( window->frameGeometry().height() );
        int width( window->frameGeometry().width() );

        // account for vertical shift
        if( yShift > 0 ) height -= 2*yShift;

        // calculate source rect
        QPoint offset( 40, 48 - 20 );
        QRect source( 0, 0, width + offset.x(), height + offset.y() );

        offset -= _backgroundPixmapOffset;
        source.translate( offset.x(), offset.y() );
        source.translate( 0, 20 - gradientHeight );

        // draw
        p->drawPixmap( QPoint( -x, -y ), _backgroundPixmap, source );

        if ( clipRect.isValid() )
        { p->restore(); }

    }

    //_____________________________________________________________
    void Helper::renderDot( QPainter* p, const QPoint& point, const QColor& baseColor )
    {

        const quint64 key( colorKey(baseColor) );
        QPixmap* pixmap( _dotCache.object( key ) );

        if( !pixmap )
        {
            pixmap = new QPixmap( highDpiPixmap( 4 ) );
            pixmap->fill( Qt::transparent );
            const qreal diameter( 1.8 );

            QPainter painter( pixmap );
            painter.setRenderHint( QPainter::Antialiasing );
            painter.setPen( Qt::NoPen );

            const QPoint center( QRect( 0, 0, 4, 4 ).center() );

            // light ellipse
            painter.setBrush( calcLightColor( baseColor ) );
            painter.drawEllipse( QRectF( center.x()-diameter/2+1.0, center.y()-diameter/2+1.0, diameter, diameter ) );

            // dark ellipse
            painter.setBrush( calcDarkColor( baseColor ).darker( 130 ) );
            painter.drawEllipse( QRectF( center.x()-diameter/2+0.5, center.y()-diameter/2+0.5, diameter, diameter ) );
            painter.end();

            // store in cache
            _dotCache.insert( key, pixmap );

        }

        p->save();
        p->translate( point - QPoint( 1,1 ) );
        p->setRenderHint( QPainter::Antialiasing );
        p->drawPixmap( QPoint( 0,0 ), *pixmap );
        p->restore();

    }

    //____________________________________________________________________
    bool Helper::lowThreshold( const QColor& color )
    {
        const quint32 key( colorKey(color) );
        ColorMap::iterator iter( _lowThreshold.find( key ) );
        if( iter != _lowThreshold.end() ) return iter.value();
        else {

            const QColor darker( KColorScheme::shade( color, KColorScheme::MidShade, 0.5 ) );
            const bool result( KColorUtils::luma( darker ) > KColorUtils::luma( color ) );
            _lowThreshold.insert( key, result );
            return result;

        }

    }

    //____________________________________________________________________
    bool Helper::highThreshold( const QColor& color )
    {
        const quint32 key( colorKey(color) );
        ColorMap::iterator iter( _highThreshold.find( key ) );
        if( iter != _highThreshold.end() ) return iter.value();
        else {

            const QColor lighter( KColorScheme::shade( color, KColorScheme::LightShade, 0.5 ) );
            const bool result( KColorUtils::luma( lighter ) < KColorUtils::luma( color ) );
            _highThreshold.insert( key, result );
            return result;

        }
    }

    //____________________________________________________________________
    QColor Helper::alphaColor( QColor color, qreal alpha )
    {
        if( alpha >= 0 && alpha < 1.0 )
        { color.setAlphaF( alpha*color.alphaF() ); }
        return color;
    }

    //____________________________________________________________________
    const QColor& Helper::backgroundRadialColor( const QColor& color )
    {
        const quint64 key( colorKey(color) );
        QColor* out( _backgroundRadialColorCache.object( key ) );
        if( !out )
        {
            if( lowThreshold( color ) ) out = new QColor( KColorScheme::shade( color, KColorScheme::LightShade, 0.0 ) );
            else if( highThreshold( color ) ) out = new QColor( color );
            else out = new QColor( KColorScheme::shade( color, KColorScheme::LightShade, _bgcontrast ) );
            _backgroundRadialColorCache.insert( key, out );
        }

        return *out;
    }

    //_________________________________________________________________________
    const QColor& Helper::backgroundTopColor( const QColor& color )
    {
        const quint64 key( colorKey(color) );
        QColor* out( _backgroundTopColorCache.object( key ) );
        if( !out )
        {
            if( lowThreshold( color ) ) out = new QColor( KColorScheme::shade( color, KColorScheme::MidlightShade, 0.0 ) );
            else {
                const qreal my( KColorUtils::luma( KColorScheme::shade( color, KColorScheme::LightShade, 0.0 ) ) );
                const qreal by( KColorUtils::luma( color ) );
                out = new QColor( KColorUtils::shade( color, ( my - by ) * _bgcontrast ) );
            }

            _backgroundTopColorCache.insert( key, out );
        }

        return *out;

    }

    //_________________________________________________________________________
    const QColor& Helper::backgroundBottomColor( const QColor& color )
    {
        const quint64 key( colorKey(color) );
        QColor* out( _backgroundBottomColorCache.object( key ) );
        if( !out )
        {
            const QColor midColor( KColorScheme::shade( color, KColorScheme::MidShade, 0.0 ) );
            if( lowThreshold( color ) ) out = new QColor( midColor );
            else {

                const qreal by( KColorUtils::luma( color ) );
                const qreal my( KColorUtils::luma( midColor ) );
                out = new QColor( KColorUtils::shade( color, ( my - by ) * _bgcontrast ) );

            }

            _backgroundBottomColorCache.insert( key, out );
        }

        return *out;

    }

    //____________________________________________________________________
    const QColor& Helper::calcLightColor( const QColor& color )
    {
        const quint64 key( colorKey(color) );
        QColor* out( _lightColorCache.object( key ) );
        if( !out )
        {
            out = new QColor( highThreshold( color ) ? color: KColorScheme::shade( color, KColorScheme::LightShade, _contrast ) );
            _lightColorCache.insert( key, out );
        }

        return *out;

    }

    //____________________________________________________________________
    const QColor& Helper::calcDarkColor( const QColor& color )
    {
        const quint64 key( colorKey(color) );
        QColor* out( _darkColorCache.object( key ) );
        if( !out )
        {
            out = new QColor( ( lowThreshold( color ) ) ?
                KColorUtils::mix( calcLightColor( color ), color, 0.3 + 0.7 * _contrast ):
                KColorScheme::shade( color, KColorScheme::MidShade, _contrast ) );
            _darkColorCache.insert( key, out );
        }

        return *out;
    }

    //____________________________________________________________________
    const QColor& Helper::calcShadowColor( const QColor& color )
    {

        const quint64 key( colorKey(color) );
        QColor* out( _shadowColorCache.object( key ) );
        if( !out )
        {
            out = new QColor( ( lowThreshold( color ) ) ?
            KColorUtils::mix( Qt::black, color, color.alphaF() ) :
            KColorScheme::shade(
                KColorUtils::mix( Qt::black, color, color.alphaF() ),
                KColorScheme::ShadowShade,
                _contrast ) );

            // make sure shadow color has the same alpha channel as the input
            out->setAlpha( color.alpha() );

            // insert in cache
            _shadowColorCache.insert( key, out );
        }

        return *out;

    }

    //____________________________________________________________________
    const QColor& Helper::backgroundColor( const QColor& color, qreal ratio )
    {

        const quint64 key( ( colorKey(color) << 32 ) | int( ratio*512 ) );
        QColor *out( _backgroundColorCache.object( key ) );
        if( !out )
        {
            if( ratio < 0.5 )
            {

                const qreal a( 2.0*ratio );
                out = new QColor( KColorUtils::mix( backgroundTopColor( color ), color, a ) );

            } else {

                const qreal a( 2.0*ratio-1 );
                out = new QColor( KColorUtils::mix( color, backgroundBottomColor( color ), a ) );

            }

            _backgroundColorCache.insert( key, out );

        }

        return *out;

    }


    //____________________________________________________________________
    QPixmap Helper::verticalGradient( const QColor& color, int height, int offset )
    {
        const quint64 key( ( colorKey(color) << 32 ) | height | 0x8000 );
        QPixmap* pixmap( _backgroundCache.object( key ) );

        if ( !pixmap )
        {
            pixmap = new QPixmap( 1, height );
            pixmap->fill( Qt::transparent );

            QLinearGradient gradient( 0, offset, 0, height );
            gradient.setColorAt( 0.0, backgroundTopColor( color ) );
            gradient.setColorAt( 0.5, color );
            gradient.setColorAt( 1.0, backgroundBottomColor( color ) );

            QPainter painter( pixmap );
            painter.fillRect( pixmap->rect(), gradient );

            painter.end();

            _backgroundCache.insert( key, pixmap );
        }

        return *pixmap;
    }

    //____________________________________________________________________
    QPixmap Helper::radialGradient( const QColor& color, int width, int height )
    {
        const quint64 key( ( colorKey(color) << 32 ) | width | 0xb000 );
        QPixmap* pixmap( _backgroundCache.object( key ) );

        if ( !pixmap )
        {
            pixmap = new QPixmap( width, height );
            pixmap->fill( Qt::transparent );

            QRadialGradient gradient( 64, height-64, 64 );
            QColor radialColor = backgroundRadialColor( color );
            radialColor.setAlpha( 255 ); gradient.setColorAt( 0, radialColor );
            radialColor.setAlpha( 101 ); gradient.setColorAt( 0.5, radialColor );
            radialColor.setAlpha( 37 );  gradient.setColorAt( 0.75, radialColor );
            radialColor.setAlpha( 0 );   gradient.setColorAt( 1, radialColor );

            QPainter painter( pixmap );
            painter.setWindow( 0, 0, 128, height );
            painter.fillRect( QRect( 0,0,128,height ), gradient );

            painter.end();

            _backgroundCache.insert( key, pixmap );
        }

        return *pixmap;
    }

    //____________________________________________________________________________________
    const QColor& Helper::decoColor( const QColor& background, const QColor& color )
    {
        const quint64 key( ( colorKey(background) << 32 ) | colorKey(color) );
        QColor* out( _decoColorCache.object( key ) );
        if( !out )
        {
            out = new QColor( KColorUtils::mix( background, color, 0.8*( 1.0 + _contrast ) ) );
            _decoColorCache.insert( key, out );
        }

        return *out;
    }

    //_______________________________________________________________________
    QRegion Helper::roundedMask( const QRect& rect, int left, int right, int top, int bottom ) const
    {
        // get rect geometry
        int x, y, w, h;
        rect.getRect( &x, &y, &w, &h );

        QRegion mask( x + 4*left, y + 0*top, w-4*( left+right ), h-0*( top+bottom ) );
        mask += QRegion( x + 0*left, y + 4*top, w-0*( left+right ), h-4*( top+bottom ) );
        mask += QRegion( x + 2*left, y + 1*top, w-2*( left+right ), h-1*( top+bottom ) );
        mask += QRegion( x + 1*left, y + 2*top, w-1*( left+right ), h-2*( top+bottom ) );

        return mask;
    }

    //______________________________________________________________________________
    QBitmap Helper::roundedMask( const QSize& size, Corners corners, qreal radius ) const
    {
        QBitmap bitmap( highDpiPixmap( size ) );
        if( corners == 0 )
        {

            bitmap.fill( Qt::color1 );

        } else {

            // initialize bitmap
            bitmap.fill( Qt::color0 );

            // setup painter
            QPainter painter( &bitmap );
            painter.setPen( Qt::NoPen );
            painter.setBrush( Qt::color1 );

            // get path
            const QPainterPath path( roundedPath( bitmap.rect(), corners, radius ) );
            painter.drawPath( path );

        }

        return bitmap;
    }

    //______________________________________________________________________________
    QPainterPath Helper::roundedPath( const QRect& rect, Corners corners, qreal radius ) const
    {

        QPainterPath path;

        // simple cases
        if( corners == 0 )
        {

            path.addRect( rect );
            return path;

        }

        if( corners == AllCorners ) {

            path.addRoundedRect( rect, radius, radius );
            return path;

        }

        const QSizeF cornerSize( 2*radius, 2*radius );

        // rotate counterclockwise
        // top left corner
        if( corners & CornerTopLeft )
        {

            path.moveTo( rect.topLeft() + QPointF( radius, 0 ) );
            path.arcTo( QRectF( rect.topLeft(), cornerSize ), 90, 90 );

        } else path.moveTo( rect.topLeft() );

        // bottom left corner
        if( corners & CornerBottomLeft )
        {

            path.lineTo( rect.bottomLeft() - QPointF( 0, radius ) );
            path.arcTo( QRectF( rect.bottomLeft() - QPointF( 0, 2*radius ), cornerSize ), 180, 90 );

        } else path.lineTo( rect.bottomLeft() );

        // bottom right corner
        if( corners & CornerBottomRight )
        {

            path.lineTo( rect.bottomRight() - QPointF( radius, 0 ) );
            path.arcTo( QRectF( rect.bottomRight() - QPointF( 2*radius, 2*radius ), cornerSize ), 270, 90 );

        } else path.lineTo( rect.bottomRight() );

        // top right corner
        if( corners & CornerTopRight )
        {

            path.lineTo( rect.topRight() + QPointF( 0, radius ) );
            path.arcTo( QRectF( rect.topRight() - QPointF( 2*radius, 0 ), cornerSize ), 0, 90 );

        } else path.lineTo( rect.topRight() );

        path.closeSubpath();
        return path;

    }

    //______________________________________________________________________
    void Helper::drawFloatFrame(
        QPainter* p, const QRect r,
        const QColor& color,
        bool drawUglyShadow, bool isActive, const QColor& frameColor, TileSet::Tiles tiles )
    {

        p->save();
        p->setRenderHint( QPainter::Antialiasing );
        const QRect frame( r.adjusted( 1,1,-1,-1 ) );
        int x,y,w,h;
        frame.getRect( &x, &y, &w, &h );

        QColor light( calcLightColor( backgroundTopColor( color ) ) );
        QColor dark( calcLightColor( backgroundBottomColor( color ) ) );
        p->setBrush( Qt::NoBrush );

        if ( drawUglyShadow )
        {

            if( isActive )
            {
                //window active - it's a glow - not a shadow
                const QColor glow( KColorUtils::mix( QColor( 128,128,128 ), frameColor, 0.7 ) );
                p->setPen( glow );

                if( tiles & TileSet::Top )
                {
                    p->drawLine( QPointF( x+4, y-0.5 ), QPointF( x+w-4, y-0.5 ) );
                    p->drawArc( QRectF( x-0.5, y-0.5, 11, 11 ),90*16, 90*16 );
                    p->drawArc( QRectF( x+w-11+0.5, y-0.5, 11, 11 ), 0, 90*16 );
                }

                if( tiles & TileSet::Left ) p->drawLine( QPointF( x-0.5, y+4 ), QPointF( x-0.5, y+h-4 ) );
                if( tiles & TileSet::Right ) p->drawLine( QPointF( x+w+0.5, y+4 ), QPointF( x+w+0.5, y+h-4 ) );

                if( tiles & TileSet::Bottom )
                {
                    if( tiles & TileSet::Left ) p->drawArc( QRectF( x-0.5, y+h-11+0.5, 11, 11 ),180*16, 90*16 );
                    if( tiles & TileSet::Right ) p->drawArc( QRectF( x+w-11+0.5, y+h-11+0.5, 11, 11 ),270*16, 90*16 );
                    p->drawLine( QPointF( x+4, y+h+0.5 ), QPointF( x+w-4, y+h+0.5 ) );
                }

                light = KColorUtils::mix( light, frameColor );
                dark = KColorUtils::mix( dark, frameColor );

            } else {

                // window inactive - draw something resembling shadow
                // fully desaturate
                const QColor shadow( KColorUtils::darken( color, 0.0, 0.0 ) );

                if( tiles & TileSet::Top )
                {
                    p->setPen( KColorUtils::darken( shadow, 0.2 ) );
                    p->drawLine( QPointF( x+4, y-0.5 ), QPointF( x+w-4, y-0.5 ) );
                    if( tiles & TileSet::Left ) p->drawArc( QRectF( x-0.5, y-0.5, 11, 11 ),90*16, 90*16 );
                    if( tiles & TileSet::Right ) p->drawArc( QRectF( x+w-11+0.5, y-0.5, 11, 11 ), 0, 90*16 );
                }

                p->setPen( KColorUtils::darken( shadow, 0.35 ) );
                if( tiles & TileSet::Left ) p->drawLine( QPointF( x-0.5, y+4 ), QPointF( x-0.5, y+h-4 ) );
                if( tiles & TileSet::Right ) p->drawLine( QPointF( x+w+0.5, y+4 ), QPointF( x+w+0.5, y+h-4 ) );

                if( tiles & TileSet::Bottom )
                {

                    p->setPen( KColorUtils::darken( shadow, 0.45 ) );
                    if( tiles & TileSet::Left ) p->drawArc( QRectF( x-0.5, y+h-11+0.5, 11, 11 ),180*16, 90*16 );
                    if( tiles & TileSet::Right ) p->drawArc( QRectF( x+w-11+0.5, y+h-11+0.5, 11, 11 ),270*16, 90*16 );
                    p->setPen( KColorUtils::darken( shadow, 0.6 ) );
                    p->drawLine( QPointF( x+4, y+h+0.5 ), QPointF( x+w-4, y+h+0.5 ) );

                }

            }
        }

        // top frame
        if( tiles & TileSet::Top )
        {
            p->setPen( QPen( light, 0.8 ) );
            p->drawLine( QPointF( x+4, y+0.6 ), QPointF( x+w-4, y+0.6 ) );
        }

        // corner and side frames
        // sides are drawn even if Top only is selected, but with a different gradient
        if( h >= 4+1.5 )
        {
            QLinearGradient lg( 0.0, y+1.5, 0.0, y+h-4 );
            lg.setColorAt( 0, light );
            lg.setColorAt( 1, alphaColor( light, 0 ) );

            if( h > 20.5 ) lg.setColorAt( qMax( 0.0, 1.0 - 12.0/( h-5.5 ) ), alphaColor( light, 0.5 ) );
            else if( h > 8.5 ) lg.setColorAt( qMax( 0.0, 3.0/( h-5.5 ) ), alphaColor( light, 0.5 ) );

            p->setPen( QPen( lg, 0.8 ) );
            if( tiles & TileSet::Left ) p->drawLine( QPointF( x+0.6, y+4 ), QPointF( x+0.6, y+h-4 ) );
            if( tiles & TileSet::Right ) p->drawLine( QPointF( x+w-0.6, y+4 ), QPointF( x+w-0.6, y+h-4 ) );
        }

        if( tiles & TileSet::Top )
        {
            const qreal offset = 0.5;
            const qreal arc( 7.0 );
            p->drawArc( QRectF( x+offset, y+offset, arc, arc ),90*16, 90*16 );
            p->drawArc( QRectF( x+w-arc-offset, y+offset, arc, arc ), 0, 90*16 );
        }

        p->restore();

    }

    //______________________________________________________________________________________
    void Helper::drawSeparator( QPainter* p, const QRect& rect, const QColor& color, Qt::Orientation orientation )
    {

        QColor light( calcLightColor( color ) );
        QColor dark( calcDarkColor( color ) );

        p->save();
        p->setRenderHint( QPainter::Antialiasing,false );

        QPoint start,end,offset;
        if ( orientation == Qt::Horizontal )
        {

            start = QPoint( rect.x(),rect.y()+rect.height()/2-1 );
            end = QPoint( rect.right(),rect.y()+rect.height()/2-1 );
            offset = QPoint( 0,1 );

        } else {

            start = QPoint( rect.x()+rect.width()/2-1,rect.y() );
            end = QPoint( rect.x()+rect.width()/2-1,rect.bottom() );
            offset = QPoint( 1,0 );
            light.setAlpha( 150 );

        }

        QLinearGradient lg( start,end );
        lg.setColorAt( 0.3, dark );
        lg.setColorAt( 0.7, dark );
        dark.setAlpha( 0 );
        lg.setColorAt( 0.0, dark );
        lg.setColorAt( 1.0, dark );
        p->setPen( QPen( lg,1 ) );

        if ( orientation == Qt::Horizontal ) p->drawLine( start,end );
        else p->drawLine( start+offset,end+offset );

        lg = QLinearGradient( start,end );
        lg.setColorAt( 0.3, light );
        lg.setColorAt( 0.7, light );
        light.setAlpha( 0 );
        lg.setColorAt( 0.0, light );
        lg.setColorAt( 1.0, light );
        p->setPen( QPen( lg,1 ) );

        if ( orientation == Qt::Horizontal ) p->drawLine( start+offset,end+offset );
        else
        {
            p->drawLine( start,end );
            p->drawLine( start+offset*2,end+offset*2 );
        }

        p->restore();
    }

    //________________________________________________________________________________________________________
    TileSet *Helper::slab( const QColor& color, const QColor& glow, qreal shade, int size )
    {
        Carbon::Cache<TileSet>::Value* cache( _slabCache.get( color ) );

        const quint64 key( ( colorKey(glow) << 32 ) | ( quint64( 256.0 * shade ) << 24 ) | size );
        TileSet *tileSet = cache->object( key );

        if ( !tileSet )
        {
            QPixmap pixmap( highDpiPixmap( size*2 ) );
            pixmap.fill( Qt::transparent );

            QPainter painter( &pixmap );
            painter.setRenderHints( QPainter::Antialiasing );
            painter.setPen( Qt::NoPen );

            const int fixedSize( 14*devicePixelRatio( pixmap ) );
            painter.setWindow( 0, 0, fixedSize, fixedSize );

            // draw all components
            if( color.isValid() ) drawShadow( painter, calcShadowColor( color ), 14 );
            if( glow.isValid() ) drawOuterGlow( painter, glow, 14 );
            if( color.isValid() ) drawSlab( painter, color, shade );

            painter.end();

            tileSet = new TileSet( pixmap,
                size, size,
                size, size,
                size-1, size,
                2, 1 );

            cache->insert( key, tileSet );

        }
        return tileSet;
    }

    //________________________________________________________________________________________________________
    TileSet *Helper::slabSunken( const QColor& color, int size )
    {
        const quint64 key( colorKey(color) << 32 | size );
        TileSet *tileSet = _slabSunkenCache.object( key );

        if ( !tileSet )
        {
            QPixmap pixmap( highDpiPixmap( size*2 ) );
            pixmap.fill( Qt::transparent );

            QPainter painter( &pixmap );
            painter.setRenderHints( QPainter::Antialiasing );
            painter.setPen( Qt::NoPen );

            const int fixedSize( 14*devicePixelRatio( pixmap ) );
            painter.setWindow( 0, 0, fixedSize, fixedSize );

            // shadow
            painter.setCompositionMode( QPainter::CompositionMode_SourceOver );
            drawInverseShadow( painter, calcShadowColor( color ), 3, 8, 0.0 );

            // contrast pixel
            {
                QColor light( calcLightColor( color ) );
                QLinearGradient blend( 0, 2, 0, 16 );
                blend.setColorAt( 0.5, Qt::transparent );
                blend.setColorAt( 1.0, light );

                painter.setBrush( Qt::NoBrush );
                painter.setPen( QPen( blend, 1 ) );
                painter.drawRoundedRect( QRectF( 2.5, 2.5, 9, 9 ), 4.0, 4.0 );
                painter.setPen( Qt::NoPen );
            }


            painter.end();

            tileSet = new TileSet( pixmap, size, size, size, size, size-1, size, 2, 1 );

            _slabSunkenCache.insert( key, tileSet );

        }

        return tileSet;

    }

    //________________________________________________________________________________________________________
    void Helper::fillSlab( QPainter& painter, const QRect& rect, int size ) const
    {
        const qreal s( qreal( size ) * ( 3.6 + ( 0.5 * _slabThickness ) ) / 7.0 );
        const QRectF r( QRectF( rect ).adjusted( s, s, -s, -s ) );
        if( !r.isValid() ) return;

        painter.drawRoundedRect( r, s/2, s/2 );
    }

    //________________________________________________________________________________________________________
    void Helper::fillButtonSlab( QPainter& painter, const QRect& r, const QColor& color, bool sunken )
    {

        painter.save();
        painter.setRenderHint( QPainter::Antialiasing );
        painter.setPen( Qt::NoPen );

        if( sunken && calcShadowColor( color ).value() > color.value() )
        {

            QLinearGradient innerGradient( 0, r.top(), 0, r.bottom() + r.height() );
            innerGradient.setColorAt( 0.0, color );
            innerGradient.setColorAt( 1.0, calcLightColor( color ) );
            painter.setBrush( innerGradient );

        } else if( sunken ) {


            QLinearGradient innerGradient( 0, r.top() - r.height(), 0, r.bottom() );
            innerGradient.setColorAt( 0.0, calcLightColor( color ) );
            innerGradient.setColorAt( 1.0, color );
            painter.setBrush( innerGradient );

        } else {

            QLinearGradient innerGradient( 0, r.top()-0.2*r.height(), 0, r.bottom()+ 0.4*r.height() );
            innerGradient.setColorAt( 0.0, calcLightColor( color ) );
            innerGradient.setColorAt( 0.6, color );
            painter.setBrush( innerGradient );

        }

        fillSlab( painter, r );
        painter.restore();

    }

    //________________________________________________________________________________________________________
    void Helper::drawInverseShadow(
        QPainter& painter, const QColor& color,
        int pad, int size, qreal fuzz ) const
    {

        const qreal m( qreal( size )*0.5 );
        const qreal offset( 0.8 );
        const qreal k0( ( m-2 ) / qreal( m+2.0 ) );
        QRadialGradient shadowGradient( pad+m, pad+m+offset, m+2 );
        for ( int i = 0; i < 8; i++ )
        {
            // sinusoidal gradient
            const qreal k1( ( qreal( 8 - i ) + k0 * qreal( i ) ) * 0.125 );
            const qreal a( ( cos( 3.14159 * i * 0.125 ) + 1.0 ) * 0.25 );
            shadowGradient.setColorAt( k1, alphaColor( color, a * _shadowGain ) );
        }
        shadowGradient.setColorAt( k0, alphaColor( color, 0.0 ) );
        painter.setBrush( shadowGradient );
        painter.drawEllipse( QRectF( pad-fuzz, pad-fuzz, size+fuzz*2.0, size+fuzz*2.0 ) );
    }

    //____________________________________________________________________
    const QWidget* Helper::checkAutoFillBackground( const QWidget* w ) const
    {
        if( !w ) return nullptr;
        if( w->autoFillBackground() ) return w;
        if( w->isWindow() ) return 0;

        for( const QWidget* parent = w->parentWidget(); parent!=0; parent = parent->parentWidget() )
        {
            if( parent->autoFillBackground() ) return parent;
            if( parent == w->window() ) break;
        }

        return nullptr;
    }

    //____________________________________________________________________
    void Helper::setHasBackgroundGradient( WId id, bool value ) const
    {

        #if CARBON_HAVE_X11
        setHasHint( id, _backgroundGradientAtom, value );
        #else
        Q_UNUSED( id );
        Q_UNUSED( value );
        #endif
        return;
    }

    //____________________________________________________________________
    bool Helper::hasBackgroundGradient( WId id ) const
    {

        #if CARBON_HAVE_X11
        return hasHint( id, _backgroundGradientAtom );
        #else
        Q_UNUSED( id );
        return false;
        #endif
    }

    //____________________________________________________________________
    void Helper::setHasBackgroundPixmap( WId id, bool value ) const
    {

        #if CARBON_HAVE_X11
        setHasHint( id, _backgroundPixmapAtom, value );
        #else
        Q_UNUSED( id );
        Q_UNUSED( value );
        #endif
        return;
    }

    //____________________________________________________________________
    bool Helper::hasBackgroundPixmap( WId id ) const
    {

        #if CARBON_HAVE_X11
        return hasHint( id, _backgroundPixmapAtom );
        #else
        Q_UNUSED( id );
        return false;
        #endif
    }

    //______________________________________________________________________________________
    QPixmap Helper::highDpiPixmap( int width, int height ) const
    {
        #if QT_VERSION >= 0x050300
        const qreal dpiRatio( qApp->devicePixelRatio() );
        QPixmap pixmap( width*dpiRatio, height*dpiRatio );
        pixmap.setDevicePixelRatio( dpiRatio );
        return pixmap;
        #else
        return QPixmap( width, height );
        #endif
    }

    //______________________________________________________________________________________
    qreal Helper::devicePixelRatio( const QPixmap& pixmap ) const
    {
        #if QT_VERSION >= 0x050300
        return pixmap.devicePixelRatio();
        #else
        Q_UNUSED(pixmap);
        return 1;
        #endif
    }

    //______________________________________________________________________________
    bool Helper::isX11( void )
    {
        #if CARBON_HAVE_X11
        #if QT_VERSION >= 0x050000
        return QX11Info::isPlatformX11();
        #else
        return true;
        #endif
        #endif

        return false;

    }

    #if CARBON_HAVE_X11

    //____________________________________________________________________
    xcb_connection_t* Helper::connection( void )
    {

        #if QT_VERSION >= 0x050000
        return QX11Info::connection();
        #else
        static xcb_connection_t* connection = nullptr;
        if( !connection )
        {
            Display* display = QX11Info::display();
            if( display ) connection = XGetXCBConnection( display );
        }
        return connection;
        #endif
    }

    //____________________________________________________________________
    xcb_atom_t Helper::createAtom( const QString& name ) const
    {
        if( !isX11() ) return 0;
        xcb_intern_atom_cookie_t cookie( xcb_intern_atom( connection(), false, name.size(), qPrintable( name ) ) );
        ScopedPointer<xcb_intern_atom_reply_t> reply( xcb_intern_atom_reply( connection(), cookie, nullptr) );
        return reply ? reply->atom:0;
    }

    #endif

    //______________________________________________________________________________________
    void Helper::drawSlab( QPainter& painter, const QColor& color, qreal shade )
    {

        const QColor light( KColorUtils::shade( calcLightColor( color ), shade ) );
        const QColor base( alphaColor( light, 0.85 ) );
        const QColor dark( KColorUtils::shade( calcDarkColor( color ), shade ) );

        // bevel, part 1
        painter.save();
        const qreal y( KColorUtils::luma( base ) );
        const qreal yl( KColorUtils::luma( light ) );
        const qreal yd( KColorUtils::luma( dark ) );
        QLinearGradient bevelGradient1( 0, 7, 0, 11 );
        bevelGradient1.setColorAt( 0.0, light );
        if ( y < yl && y > yd )
        {
            // no middle when color is very light/dark
            bevelGradient1.setColorAt( 0.5, base );
        }

        bevelGradient1.setColorAt( 0.9, base );
        painter.setBrush( bevelGradient1 );
        painter.drawRoundedRect( QRectF( 3.0,3.0,8.0,8.0 ), 3.5, 3.5 );

        // bevel, part 2
        if ( _slabThickness > 0.0 )
        {

            QLinearGradient bevelGradient2( 0, 6, 0, 19 );
            bevelGradient2.setColorAt( 0.0, light );
            bevelGradient2.setColorAt( 0.9, base );
            painter.setBrush( bevelGradient2 );
            painter.drawEllipse( QRectF( 3.6,3.6,6.8,6.8 ) );

        }

        // inside mask
        painter.setCompositionMode( QPainter::CompositionMode_DestinationOut );
        painter.setBrush( Qt::black );

        const qreal ic( 3.6 + 0.5*_slabThickness );
        const qreal is( 14.0 - 2.0*ic );
        painter.drawEllipse( QRectF( ic, ic, is, is ) );
        painter.restore();
    }

    //___________________________________________________________________________________________
    void Helper::drawShadow( QPainter& painter, const QColor& color, int size )
    {
        const qreal m( qreal( size-2 )*0.5 );
        const qreal offset( 0.8 );
        const qreal k0( ( m-4.0 ) / m );

        QRadialGradient shadowGradient( m+1.0, m+offset+1.0, m );
        for ( int i = 0; i < 8; i++ )
        {

            // sinusoidal gradient
            const qreal k1( ( k0 * qreal( 8 - i ) + qreal( i ) ) * 0.125 );
            const qreal a( ( cos( M_PI * i * 0.125 ) + 1.0 ) * 0.30 );
            shadowGradient.setColorAt( k1, alphaColor( color, a * _shadowGain ) );

        }

        shadowGradient.setColorAt( 1.0, alphaColor( color, 0.0 ) );
        painter.save();
        painter.setBrush( shadowGradient );
        painter.drawEllipse( QRectF( 0, 0, size, size ) );
        painter.restore();

    }

    //_______________________________________________________________________
    void Helper::drawOuterGlow( QPainter& painter, const QColor& color, int size )
    {

        const QRectF r( 0, 0, size, size );
        const qreal m( qreal( size )*0.5 );
        const qreal width( 3 );

        const qreal bias( _glowBias * qreal( 14 )/size );

        // k0 is located at width - bias from the outer edge
        const qreal gm( m + bias - 0.9 );
        const qreal k0( ( m-width+bias ) / gm );
        QRadialGradient glowGradient( m, m, gm );
        for ( int i = 0; i < 8; i++ )
        {

            // k1 grows linearly from k0 to 1.0
            const qreal k1( k0 + qreal( i )*( 1.0-k0 )/8.0 );

            // a folows sqrt curve
            const qreal a( 1.0 - sqrt( qreal( i )/8 ) );
            glowGradient.setColorAt( k1, alphaColor( color, a ) );
        }

        // glow
        painter.save();
        painter.setBrush( glowGradient );
        painter.drawEllipse( r );

        // inside mask
        painter.setCompositionMode( QPainter::CompositionMode_DestinationOut );
        painter.setBrush( Qt::black );
        painter.drawEllipse( r.adjusted( width+0.5, width+0.5, -width-1, -width-1 ) );
        painter.restore();

    }

    #if CARBON_HAVE_X11

    //____________________________________________________________________
    void Helper::setHasHint( xcb_window_t id, xcb_atom_t atom, bool value ) const
    {
        if( !isX11() ) return;

        // check window id
        if( !id ) return;

        quint32 uLongValue( value );
        xcb_change_property( connection(), XCB_PROP_MODE_REPLACE, id, atom, XCB_ATOM_CARDINAL, 32, 1, &uLongValue );
        xcb_flush( connection() );
        return;

    }

    //____________________________________________________________________
    bool Helper::hasHint( xcb_window_t id, xcb_atom_t atom ) const
    {
        if( !isX11() ) return false;

        // check window id
        if( !id ) return false;

        xcb_get_property_cookie_t cookie( xcb_get_property( connection(), 0, id, atom, XCB_ATOM_CARDINAL, 0, 1) );
        ScopedPointer<xcb_get_property_reply_t> reply( xcb_get_property_reply( connection(), cookie, nullptr ) );

        return reply && xcb_get_property_value_length( reply.data() ) && reinterpret_cast<int32_t*>(xcb_get_property_value( reply.data() ) )[0];

    }

    #endif


    //____________________________________________________________________
    void Helper::init( void )
    {

        #if CARBON_USE_KDE4
        _contrast = KGlobalSettings::contrastF( _config );
        #else
        _contrast = KColorScheme::contrastF( _config );
        #endif

        // background contrast is calculated so that it is 0.9
        // when KGlobalSettings contrast value of 0.7
        _bgcontrast = qMin( 1.0, 0.9*_contrast/0.7 );

        _backgroundCache.setMaxCost( 64 );

        #if CARBON_HAVE_X11
        if( isX11() )
        {

            _backgroundGradientAtom = createAtom( QStringLiteral( "_KDE_CARBON_BACKGROUND_GRADIENT" ) );
            _backgroundPixmapAtom = createAtom( QStringLiteral( "_KDE_CARBON_BACKGROUND_PIXMAP" ) );

        } else {

            _backgroundGradientAtom = 0;
            _backgroundPixmapAtom = 0;

        }

        #endif

    }

}
