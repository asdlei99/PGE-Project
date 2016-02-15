/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2014-2015 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common_features/logger.h>
#include <common_features/mainwinconnect.h>
#include <common_features/graphics_funcs.h>
#include <main_window/dock/lvl_warp_props.h>

#include "item_block.h"
#include "item_bgo.h"
#include "item_npc.h"
#include "item_water.h"
#include "item_door.h"
#include "../newlayerbox.h"


ItemDoor::ItemDoor(QGraphicsItem *parent)
    : LvlBaseItem(parent)
{
    construct();
}

ItemDoor::ItemDoor(LvlScene *parentScene, QGraphicsItem *parent)
    : LvlBaseItem(parentScene, parent)
{
    construct();
    if(!parentScene) return;
    setScenePoint(parentScene);
    scene->addItem(this);
    setLocked(scene->lock_door);
}

void ItemDoor::construct()
{
    gridSize=16;
    itemSize = QSize(32,32);
    this->setData(ITEM_WIDTH, 32);
    this->setData(ITEM_HEIGHT, 32);
    grp = NULL;
    doorLabel = NULL;
}

ItemDoor::~ItemDoor()
{
    if(doorLabel!=NULL) delete doorLabel;
    if(grp!=NULL) delete grp;
    scene->unregisterElement(this);
}



void ItemDoor::contextMenu( QGraphicsSceneMouseEvent * mouseEvent )
{
    scene->contextMenuOpened = true; //bug protector

    //Remove selection from non-bgo items
    if(!this->isSelected())
    {
        scene->clearSelection();
        this->setSelected(true);
    }

    this->setSelected(1);
    QMenu ItemMenu;

    QAction *openLvl = ItemMenu.addAction(tr("Open target level: %1").arg(doorData.lname).replace("&", "&&&"));
    openLvl->setVisible( (!doorData.lname.isEmpty()) && (QFile(scene->LvlData->path + "/" + doorData.lname).exists()) );
    openLvl->deleteLater();

    /*************Layers*******************/
    QMenu * LayerName =     ItemMenu.addMenu(tr("Layer: ")+QString("[%1]").arg(doorData.layer).replace("&", "&&&"));
    QAction *setLayer;
    QList<QAction *> layerItems;

    QAction * newLayer =    LayerName->addAction(tr("Add to new layer..."));
        LayerName->addSeparator()->deleteLater();;
    foreach(LevelLayer layer, scene->LvlData->layers)
    {
        //Skip system layers
        if((layer.name=="Destroyed Blocks")||(layer.name=="Spawned NPCs")) continue;

        setLayer = LayerName->addAction( layer.name.replace("&", "&&&")+((layer.hidden)?" [hidden]":"") );
        setLayer->setData(layer.name);
        setLayer->setCheckable(true);
        setLayer->setEnabled(true);
        setLayer->setChecked( layer.name==doorData.layer );
        layerItems.push_back(setLayer);
    }
    ItemMenu.addSeparator();
    /*************Layers*end***************/

    QAction *jumpTo=NULL;
    if(this->data(ITEM_TYPE).toString()=="Door_enter")
    {
        jumpTo =                ItemMenu.addAction(tr("Jump to exit"));
        jumpTo->setVisible( (doorData.isSetIn)&&(doorData.isSetOut) );
    }
    else
    if(this->data(ITEM_TYPE).toString()=="Door_exit")
    {
        jumpTo =                ItemMenu.addAction(tr("Jump to entrance"));
        jumpTo->setVisible( (doorData.isSetIn)&&(doorData.isSetOut) );
    }
                                ItemMenu.addSeparator();
    QAction * NoTransport =     ItemMenu.addAction(tr("No Vehicles"));
        NoTransport->setCheckable(true);
        NoTransport->setChecked( doorData.novehicles );

    QAction * AllowNPC =        ItemMenu.addAction(tr("Allow NPC"));
        AllowNPC->setCheckable(true);
        AllowNPC->setChecked( doorData.allownpc );

    QAction * Locked =          ItemMenu.addAction(tr("Locked"));
        Locked->setCheckable(true);
        Locked->setChecked( doorData.locked );

    /*************Copy Preferences*******************/
    ItemMenu.addSeparator();
    QMenu * copyPreferences =   ItemMenu.addMenu(tr("Copy preferences"));
    QAction *copyPosXY =        copyPreferences->addAction(tr("Position: X, Y"));
    QAction *copyPosXYWH =      copyPreferences->addAction(tr("Position: X, Y, Width, Height"));
    QAction *copyPosLTRB =      copyPreferences->addAction(tr("Position: Left, Top, Right, Bottom"));
    /*************Copy Preferences*end***************/

                                ItemMenu.addSeparator();
    QAction *remove =           ItemMenu.addAction(tr("Remove"));

                                ItemMenu.addSeparator();
    QAction *props =            ItemMenu.addAction(tr("Properties..."));

    /*****************Waiting for answer************************/
    QAction *selected = ItemMenu.exec(mouseEvent->screenPos());
    /***********************************************************/

    if(!selected)
        return;

    if(selected==openLvl)
    {
        MainWinConnect::pMainWin->OpenFile(scene->LvlData->path + "/" + doorData.lname);
    }
    else
    if(selected==jumpTo)
    {
        //scene->doCopy = true ;
        if(this->data(ITEM_TYPE).toString()=="Door_enter")
        {
            if(doorData.isSetOut)
            MainWinConnect::pMainWin->activeLvlEditWin()->goTo(doorData.ox, doorData.oy, true, QPoint(0, 0), true);
        }
        else
        if(this->data(ITEM_TYPE).toString()=="Door_exit")
        {
            if(doorData.isSetIn)
            MainWinConnect::pMainWin->activeLvlEditWin()->goTo(doorData.ix, doorData.iy, true, QPoint(0, 0), true);
        }
    }
    else
    if(selected==NoTransport)
    {
        LevelData modDoors;
        foreach(QGraphicsItem * SelItem, scene->selectedItems() )
        {
            if((SelItem->data(ITEM_TYPE).toString()=="Door_exit")||(SelItem->data(ITEM_TYPE).toString()=="Door_enter"))
            {
                if(SelItem->data(ITEM_TYPE).toString()=="Door_exit"){
                    LevelDoor door = ((ItemDoor *) SelItem)->doorData;
                    door.isSetOut = true;
                    door.isSetIn = false;
                    modDoors.doors.push_back(door);
                }else if(SelItem->data(ITEM_TYPE).toString()=="Door_enter"){
                    LevelDoor door = ((ItemDoor *) SelItem)->doorData;
                    door.isSetOut = false;
                    door.isSetIn = true;
                    modDoors.doors.push_back(door);
                }
                ((ItemDoor *) SelItem)->doorData.novehicles=NoTransport->isChecked();
                ((ItemDoor *) SelItem)->arrayApply();
            }
        }
        scene->addChangeSettingsHistory(modDoors, HistorySettings::SETTING_NOYOSHI, QVariant(NoTransport->isChecked()));
        MainWinConnect::pMainWin->dock_LvlWarpProps->setDoorData(-2);
    }
    else
    if(selected==AllowNPC)
    {
        LevelData modDoors;
        foreach(QGraphicsItem * SelItem, scene->selectedItems() )
        {
            if((SelItem->data(ITEM_TYPE).toString()=="Door_exit")||(SelItem->data(ITEM_TYPE).toString()=="Door_enter"))
            {
                if(SelItem->data(ITEM_TYPE).toString()=="Door_exit") {
                    LevelDoor door = ((ItemDoor *) SelItem)->doorData;
                    door.isSetOut = true;
                    door.isSetIn = false;
                    modDoors.doors.push_back(door);
                } else if(SelItem->data(ITEM_TYPE).toString()=="Door_enter"){
                    LevelDoor door = ((ItemDoor *) SelItem)->doorData;
                    door.isSetOut = false;
                    door.isSetIn = true;
                    modDoors.doors.push_back(door);
                }
                ((ItemDoor *) SelItem)->doorData.allownpc=AllowNPC->isChecked();
                ((ItemDoor *) SelItem)->arrayApply();
            }
        }
        scene->addChangeSettingsHistory(modDoors, HistorySettings::SETTING_ALLOWNPC, QVariant(AllowNPC->isChecked()));
        MainWinConnect::pMainWin->dock_LvlWarpProps->setDoorData(-2);
    }
    else
    if(selected==Locked)
    {
        LevelData modDoors;
        foreach(QGraphicsItem * SelItem, scene->selectedItems() )
        {
            if((SelItem->data(ITEM_TYPE).toString()=="Door_exit")||(SelItem->data(ITEM_TYPE).toString()=="Door_enter"))
            {
                if(SelItem->data(ITEM_TYPE).toString()=="Door_exit"){
                    LevelDoor door = ((ItemDoor *) SelItem)->doorData;
                    door.isSetOut = true;
                    door.isSetIn = false;
                    modDoors.doors.push_back(door);
                }else if(SelItem->data(ITEM_TYPE).toString()=="Door_enter"){
                    LevelDoor door = ((ItemDoor *) SelItem)->doorData;
                    door.isSetOut = false;
                    door.isSetIn = true;
                    modDoors.doors.push_back(door);
                }
                ((ItemDoor *) SelItem)->doorData.locked=Locked->isChecked();
                ((ItemDoor *) SelItem)->arrayApply();
            }
        }
        scene->addChangeSettingsHistory(modDoors, HistorySettings::SETTING_LOCKED, QVariant(Locked->isChecked()));
        MainWinConnect::pMainWin->dock_LvlWarpProps->setDoorData(-2);
    }
    else
    if(selected==copyPosXY)
    {
        QApplication::clipboard()->setText(
                            QString("X=%1; Y=%2;")
                               .arg(direction==D_Entrance ? doorData.ix : doorData.ox)
                               .arg(direction==D_Entrance ? doorData.iy : doorData.oy)
                               );
        MainWinConnect::pMainWin->showStatusMsg(tr("Preferences has been copied: %1").arg(QApplication::clipboard()->text()));
    }
    else
    if(selected==copyPosXYWH)
    {
        QApplication::clipboard()->setText(
                            QString("X=%1; Y=%2; W=%3; H=%4;")
                               .arg(direction==D_Entrance ? doorData.ix : doorData.ox)
                               .arg(direction==D_Entrance ? doorData.iy : doorData.oy)
                               .arg(itemSize.width())
                               .arg(itemSize.height())
                               );
        MainWinConnect::pMainWin->showStatusMsg(tr("Preferences has been copied: %1").arg(QApplication::clipboard()->text()));
    }
    else
    if(selected==copyPosLTRB)
    {
        QApplication::clipboard()->setText(
                            QString("Left=%1; Top=%2; Right=%3; Bottom=%4;")
                               .arg(direction==D_Entrance ? doorData.ix : doorData.ox)
                               .arg(direction==D_Entrance ? doorData.iy : doorData.oy)
                               .arg((direction==D_Entrance ? doorData.ix : doorData.ox)+itemSize.width())
                               .arg((direction==D_Entrance ? doorData.iy : doorData.oy)+itemSize.height())
                               );
        MainWinConnect::pMainWin->showStatusMsg(tr("Preferences has been copied: %1").arg(QApplication::clipboard()->text()));
    }
    else
    if(selected==remove)
    {
        scene->removeSelectedLvlItems();
    }
    else
    if(selected==props)
    {
        MainWinConnect::pMainWin->dock_LvlWarpProps->SwitchToDoor(doorData.array_id);
    }
    else
    if(selected==newLayer)
    {
        scene->setLayerToSelected();
        scene->applyLayersVisible();
    }
    else
    {
        //Fetch layers menu
        foreach(QAction * lItem, layerItems)
        {
            if(selected==lItem)
            {
                //FOUND!!!
                scene->setLayerToSelected(lItem->data().toString());
                scene->applyLayersVisible();
                MainWinConnect::pMainWin->dock_LvlWarpProps->setDoorData(-2);
                break;
            }//Find selected layer's item
        }
    }
}


///////////////////MainArray functions/////////////////////////////
void ItemDoor::setLayer(QString layer)
{
    foreach(LevelLayer lr, scene->LvlData->layers)
    {
        if(lr.name==layer)
        {
            doorData.layer = layer;
            this->setVisible(!lr.hidden);
            arrayApply();
        break;
        }
    }
}

void ItemDoor::arrayApply()
{
    bool found=false;

    if((direction==D_Entrance) && doorData.isSetIn)
    {
        doorData.ix = qRound(this->scenePos().x());
        doorData.iy = qRound(this->scenePos().y());
    }
    else if((direction==D_Exit) && doorData.isSetOut )
    {
        doorData.ox = qRound(this->scenePos().x());
        doorData.oy = qRound(this->scenePos().y());
    }
    /** Explanation for cramps-man why was that dumb bug:
     *
     * when we killed point, doorData.isSetIn is false.
     * so:
     * direction==D_Entrance is true, but isSetIN - false
     * else isSetOut - true
     * so: we are applying ENTRANCE's physical coordinates into EXIT :P
     * but that will NOT appear in file if we will just make dummy modify
     * of exit point and it's true value will overwrire invalid data.
     *
     * To prevent this crap, need to also add condition
     * to check "is this point is exit"?
     * so, even if entrance point marked as "false" because "not placed" flag
     * exit point's value will not be overwritten
     *
     */

    if(doorData.index < (unsigned int)scene->LvlData->doors.size())
    { //Check index
        if(doorData.array_id == scene->LvlData->doors[doorData.index].array_id)
        {
            found=true;
        }
    }

    //Apply current data in main array
    if(found)
    { //directlry
        scene->LvlData->doors[doorData.index] = doorData; //apply current bgoData
    }
    else
    for(int i=0; i<scene->LvlData->doors.size(); i++)
    { //after find it into array
        if(scene->LvlData->doors[i].array_id == doorData.array_id)
        {
            doorData.index = i;
            scene->LvlData->doors[i] = doorData;
            break;
        }
    }

    //Sync data to his pair door item
    if(direction==D_Entrance)
    {
        if(doorData.isSetOut)
        {
            foreach(QGraphicsItem * door, scene->items())
            {
                if((door->data(ITEM_TYPE).toString()=="Door_exit")&&((unsigned int)door->data(ITEM_ARRAY_ID).toInt()==doorData.array_id))
                {
                    ((ItemDoor *)door)->doorData = doorData;
                    break;
                }
            }
        }
    }
    else
    {
        if(doorData.isSetIn)
        {
            foreach(QGraphicsItem * door, scene->items())
            {
                if((door->data(ITEM_TYPE).toString()=="Door_enter")&&((unsigned int)door->data(ITEM_ARRAY_ID).toInt()==doorData.array_id))
                {
                    ((ItemDoor *)door)->doorData = doorData;
                    break;
                }
            }
        }

    }

    //Update R-tree innex
    scene->unregisterElement(this);
    scene->registerElement(this);
}

void ItemDoor::removeFromArray()
{
    if(direction==D_Entrance)
    {
        doorData.isSetIn=false;
        doorData.ix = 0;
        doorData.iy = 0;
    }
    else
    {
        doorData.isSetOut=false;
        doorData.ox = 0;
        doorData.oy = 0;
    }
    arrayApply();
}


void ItemDoor::returnBack()
{
    if(direction==D_Entrance)
        this->setPos(doorData.ix, doorData.iy);
    else
        this->setPos(doorData.ox, doorData.oy);
}

QPoint ItemDoor::gridOffset()
{
    return QPoint(gridOffsetX, gridOffsetY);
}

int ItemDoor::getGridSize()
{
    return gridSize;
}

QPoint ItemDoor::sourcePos()
{
    if(direction==D_Entrance)
        return QPoint(doorData.ix, doorData.iy);
    else
        return QPoint(doorData.ox, doorData.oy);
}

bool ItemDoor::itemTypeIsLocked()
{
    if(!scene)
        return false;
    return scene->lock_door;
}

void ItemDoor::setDoorData(LevelDoor inD, int doorDir, bool init)
{
    doorData = inD;
    direction = doorDir;

    long ix, iy, ox, oy;
    QColor cEnter(qRgb(0xff,0x00,0x7f));
    QColor cExit(qRgb(0xc4,0x00,0x62));//c40062
    cEnter.setAlpha(50);
    cExit.setAlpha(50);

    ix = doorData.ix;
    iy = doorData.iy;
    ox = doorData.ox;
    oy = doorData.oy;

    doorLabel = new QGraphicsPixmapItem(GraphicsHelps::drawDegitFont(doorData.array_id));

    if(direction==D_Entrance)
    {
        doorData.isSetIn=true;
        _brush = QBrush(cEnter);
        _pen = QPen(QBrush(QColor(qRgb(0xff,0x00,0x7f))), 2,Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
        doorLabel->setPos(ix+2, iy+2);

        setPos(ix, iy);
        setData(ITEM_TYPE, "Door_enter"); // ObjType
    }
    else
    {
        doorData.isSetOut=true;
        _brush = QBrush(cExit);
        _pen = QPen(QBrush(QColor(qRgb(0xc4,0x00,0x62))), 2,Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
        doorLabel->setPos(ox+16, oy+16);

        setPos(ox, oy);
        setData(ITEM_TYPE, "Door_exit"); // ObjType
    }
    grp->addToGroup(doorLabel);

    this->setFlag(QGraphicsItem::ItemIsSelectable, (!scene->lock_door));
    this->setFlag(QGraphicsItem::ItemIsMovable, (!scene->lock_door));

    doorLabel->setZValue(scene->Z_sys_door+0.0000002);

    this->setData(ITEM_ID, QString::number(0) );
    this->setData(ITEM_ARRAY_ID, QString::number(doorData.array_id) );

    this->setZValue(scene->Z_sys_door);

    if(!init)
    {
        arrayApply();
    }

    scene->unregisterElement(this);
    scene->registerElement(this);
}

QRectF ItemDoor::boundingRect() const
{
    return QRectF(-1,-1,itemSize.width()+2,itemSize.height()+2);
}

void ItemDoor::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setBrush(_brush);
    painter->setPen(_pen);
    painter->drawRect(1, 1, itemSize.width()-2, itemSize.height()-2);

    if(this->isSelected())
    {
        painter->setPen(QPen(QBrush(Qt::black), 2, Qt::SolidLine));
        painter->drawRect(1,1,itemSize.width()-2,itemSize.height()-2);
        painter->setPen(QPen(QBrush(Qt::white), 2, Qt::DotLine));
        painter->drawRect(1,1,itemSize.width()-2,itemSize.height()-2);
    }
}

void ItemDoor::setScenePoint(LvlScene *theScene)
{
    LvlBaseItem::setScenePoint(theScene);
    if(grp) delete grp;
    grp = new QGraphicsItemGroup(this);
    doorLabel = NULL;
}

