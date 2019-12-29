#include <msp430.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <shape.h>
#include "player.h"

//This class was inspired by Fernie Marquez he adviced me on how to create this shape

void playerGetBounds(const player *player, const Vec2 *center, Region *bounds){
  const Vec2 half ={5/2,7/2};
  vec2Sub(&bounds->topLeft,center,&half);
  vec2Add(&bounds->botRight,center,&half);
}

int drawPlayer(const player *player, const Vec2 *center, const Vec2 *pixel){
  Vec2 relPos;
  int col,row,inBounds=0;
  vec2Sub(&relPos, pixel, center);
  row=relPos.axes[1];col=relPos.axes[0];
  if(row==-1){
    if(-3<col&&col<3){
      inBounds=1;
    }
    else{
      inBounds=0;
    }
  }
  switch (col){
  case 0:
    if(-4<row&&row<2){
      inBounds=1;
    }
    else{
      inBounds=0;
    }
    break;
  
  case -2:
    if(row==0){
      inBounds=1;
    }
    break;
  case -1:
    if(row>0&&row<4){
      inBounds=1;
    }
    break;
  case 1:
    if(row>0&&row<4){
      inBounds=1;
    }
    break;
  default:
    break;
  }
  return inBounds;
}
