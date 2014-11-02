#include "mode_square.h"

#include "../lvlscene.h"
#include "../../common_features/mainwinconnect.h"
#include "../../common_features/item_rectangles.h"
#include "../lvl_item_placing.h"

#include "../item_bgo.h"
#include "../item_block.h"
#include "../item_npc.h"
#include "../item_water.h"
#include "../item_playerpoint.h"
#include "../item_door.h"

ModeSquare::ModeSquare(QGraphicsScene *parentScene, QObject *parent)
    : EditMode("Square", parentScene, parent)
{
    drawStartPos = QPointF(0,0);
}

ModeSquare::~ModeSquare()
{}

void ModeSquare::set()
{
    if(!scene) return;
    LvlScene *s = dynamic_cast<LvlScene *>(scene);

    s->EraserEnabled=false;
    s->PasteFromBuffer=false;
    s->DrawMode=true;
    s->disableMoveItems=false;

    s->clearSelection();
    s->resetResizers();

    s->_viewPort->setInteractive(true);
    s->_viewPort->setCursor(Qt::CrossCursor);
    s->_viewPort->setDragMode(QGraphicsView::NoDrag);
    s->_viewPort->setRenderHint(QPainter::Antialiasing, true);
    s->_viewPort->viewport()->setMouseTracking(true);
}

void ModeSquare::mousePress(QGraphicsSceneMouseEvent *mouseEvent)
{
    if(!scene) return;
    LvlScene *s = dynamic_cast<LvlScene *>(scene);

    if( mouseEvent->buttons() & Qt::RightButton )
    {
        item_rectangles::clearArray();
        MainWinConnect::pMainWin->on_actionSelect_triggered();
        return;
    }

    s->last_block_arrayID=s->LvlData->blocks_array_id;
    s->last_bgo_arrayID=s->LvlData->bgo_array_id;
    s->last_npc_arrayID=s->LvlData->npc_array_id;

    WriteToLog(QtDebugMsg, QString("Square mode %1").arg(s->EditingMode));
    if(s->cursor){
        drawStartPos = QPointF(s->applyGrid( mouseEvent->scenePos().toPoint(),
                                          LvlPlacingItems::gridSz,
                                          LvlPlacingItems::gridOffset));
        s->cursor->setPos( drawStartPos );
        s->cursor->setVisible(true);

        QPoint hw = s->applyGrid( mouseEvent->scenePos().toPoint(),
                               LvlPlacingItems::gridSz,
                               LvlPlacingItems::gridOffset);

        QSize hs = QSize( (long)fabs(drawStartPos.x() - hw.x()),  (long)fabs( drawStartPos.y() - hw.y() ) );
        dynamic_cast<QGraphicsRectItem *>(s->cursor)->setRect(0,0, hs.width(), hs.height());
    }
}

void ModeSquare::mouseMove(QGraphicsSceneMouseEvent *mouseEvent)
{
    if(!scene) return;
    LvlScene *s = dynamic_cast<LvlScene *>(scene);

    if(!LvlPlacingItems::layer.isEmpty() && LvlPlacingItems::layer!="Default")
        s->setMessageBoxItem(true, mouseEvent->scenePos(), LvlPlacingItems::layer + ", " +
                     QString::number( mouseEvent->scenePos().toPoint().x() ) + "x" +
                     QString::number( mouseEvent->scenePos().toPoint().y() )
                      );
    else
        s->setMessageBoxItem(false);

        if(s->cursor)
        {
            if(s->cursor->isVisible())
            {
                QPoint hw = s->applyGrid( mouseEvent->scenePos().toPoint(),
                                       LvlPlacingItems::gridSz,
                                       LvlPlacingItems::gridOffset);

                QSize hs = QSize( (long)fabs(drawStartPos.x() - hw.x()),  (long)fabs( drawStartPos.y() - hw.y() ) );


                dynamic_cast<QGraphicsRectItem *>(s->cursor)->setRect(0,0, hs.width(), hs.height());
                dynamic_cast<QGraphicsRectItem *>(s->cursor)->setPos(
                            ((hw.x() < drawStartPos.x() )? hw.x() : drawStartPos.x()),
                            ((hw.y() < drawStartPos.y() )? hw.y() : drawStartPos.y())
                            );

                if(((s->placingItem==LvlScene::PLC_Block)&&(!LvlPlacingItems::sizableBlock))||
                        (s->placingItem==LvlScene::PLC_BGO))
                {
                item_rectangles::drawMatrix(s, QRect (dynamic_cast<QGraphicsRectItem *>(s->cursor)->x(),
                                                         dynamic_cast<QGraphicsRectItem *>(s->cursor)->y(),
                                                         dynamic_cast<QGraphicsRectItem *>(s->cursor)->rect().width(),
                                                         dynamic_cast<QGraphicsRectItem *>(s->cursor)->rect().height()),
                                            QSize(LvlPlacingItems::itemW, LvlPlacingItems::itemH)
                                            );
                }
            }
        }

}

void ModeSquare::mouseRelease(QGraphicsSceneMouseEvent *mouseEvent)
{
    Q_UNUSED(mouseEvent);

    if(!scene) return;
    LvlScene *s = dynamic_cast<LvlScene *>(scene);

    if(s->cursor)
    {

        // /////////// Don't draw with zero width or height //////////////
        if( (dynamic_cast<QGraphicsRectItem *>(s->cursor)->rect().width()==0) ||
          (dynamic_cast<QGraphicsRectItem *>(s->cursor)->rect().height()==0))
        {
            s->cursor->hide();
            return;
        }
        // ///////////////////////////////////////////////////////////////

        switch(s->placingItem)
        {
        case LvlScene::PLC_Water:
            {
                LvlPlacingItems::waterSet.quicksand = (LvlPlacingItems::waterType==1);

                LvlPlacingItems::waterSet.x = s->cursor->scenePos().x();
                LvlPlacingItems::waterSet.y = s->cursor->scenePos().y();
                LvlPlacingItems::waterSet.w = dynamic_cast<QGraphicsRectItem *>(s->cursor)->rect().width();
                LvlPlacingItems::waterSet.h = dynamic_cast<QGraphicsRectItem *>(s->cursor)->rect().height();
                //here define placing water item.
                s->LvlData->physenv_array_id++;

                LvlPlacingItems::waterSet.array_id = s->LvlData->physenv_array_id;
                s->LvlData->physez.push_back(LvlPlacingItems::waterSet);

                s->placeWater(LvlPlacingItems::waterSet, true);
                LevelData plWater;
                plWater.physez.push_back(LvlPlacingItems::waterSet);
                s->addPlaceHistory(plWater);
                s->Debugger_updateItemList();
                break;
            }
        case LvlScene::PLC_Block:
            {
                //LvlPlacingItems::waterSet.quicksand = (LvlPlacingItems::waterType==1);
                if(LvlPlacingItems::sizableBlock)
                {
                    LvlPlacingItems::blockSet.x = s->cursor->scenePos().x();
                    LvlPlacingItems::blockSet.y = s->cursor->scenePos().y();
                    LvlPlacingItems::blockSet.w = dynamic_cast<QGraphicsRectItem *>(s->cursor)->rect().width();
                    LvlPlacingItems::blockSet.h = dynamic_cast<QGraphicsRectItem *>(s->cursor)->rect().height();
                    //here define placing water item.
                    s->LvlData->blocks_array_id++;

                    LvlPlacingItems::blockSet.array_id = s->LvlData->blocks_array_id;
                    s->LvlData->blocks.push_back(LvlPlacingItems::blockSet);

                    s->placeBlock(LvlPlacingItems::blockSet, true);
                    LevelData plSzBlock;
                    plSzBlock.blocks.push_back(LvlPlacingItems::blockSet);
                    s->addPlaceHistory(plSzBlock);
                    s->Debugger_updateItemList();
                    break;
                }
                else
                {
                    QPointF p = ((QGraphicsRectItem *)(s->cursor))->scenePos();
                    QSizeF sz = ((QGraphicsRectItem *)(s->cursor))->rect().size();

                    WriteToLog(QtDebugMsg, "Get collision buffer");

                    s->collisionCheckBuffer = s->items(QRectF(
                                p.x()-10, p.y()-10,
                                sz.width()+20, sz.height()+20),
                                Qt::IntersectsItemBoundingRect);
                    if(s->collisionCheckBuffer.isEmpty())
                        s->emptyCollisionCheck = true;
                    else
                        s->prepareCollisionBuffer();

                    WriteToLog(QtDebugMsg, "Placing");
                    s->placeItemsByRectArray();

                    WriteToLog(QtDebugMsg, "clear collision buffer");
                    s->emptyCollisionCheck = false;
                    s->collisionCheckBuffer.clear();
                    WriteToLog(QtDebugMsg, "Done");

                    s->Debugger_updateItemList();
                    break;
                }
            }
        case LvlScene::PLC_BGO:
            {
                QPointF p = ((QGraphicsRectItem *)(s->cursor))->scenePos();
                QSizeF sz = ((QGraphicsRectItem *)(s->cursor))->rect().size();

                s->collisionCheckBuffer = s->items(QRectF(
                            p.x()-10, p.y()-10,
                            sz.width()+20, sz.height()+20),
                            Qt::IntersectsItemBoundingRect);

                if(s->collisionCheckBuffer.isEmpty())
                    s->emptyCollisionCheck = true;
                else
                    s->prepareCollisionBuffer();

                s->placeItemsByRectArray();

                s->emptyCollisionCheck = false;
                s->collisionCheckBuffer.clear();

                s->Debugger_updateItemList();
             break;
            }
        }
        s->LvlData->modified = true;

        s->cursor->hide();
    }
}

void ModeSquare::keyPress(QKeyEvent *keyEvent)
{
    Q_UNUSED(keyEvent);
}

void ModeSquare::keyRelease(QKeyEvent *keyEvent)
{
    Q_UNUSED(keyEvent);
    switch(keyEvent->key())
    {
        case (Qt::Key_Escape):
            item_rectangles::clearArray();
            MainWinConnect::pMainWin->on_actionSelect_triggered();
            break;
        default:
            break;
    }
}
