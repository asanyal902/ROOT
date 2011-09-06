#import <UIKit/UIKit.h>

@class ScrollViewWithPadView;
@class ObjectInspector;
@class ObjectShortcut;
@class EditorView;
@class PadView;

namespace ROOT_iOS {
//Pad to draw object.
class Pad;

}

//ROOT's object to draw.
class TObject;

@interface ROOTObjectController : UIViewController <UIScrollViewDelegate> {
   EditorView *editorView;
   ObjectInspector *objectInspector;
   
   IBOutlet ScrollViewWithPadView *scrollView;
   
   PadView *padView;//View for pad.
   ROOT_iOS::Pad *pad;
   TObject *rootObject;
   TObject *selectedObject;
   
   unsigned currentEditors;
   
   BOOL zoomed;
}

@property (nonatomic, retain) UIScrollView *scrollView;

- (void) setObjectFromShortcut : (ObjectShortcut *)object;
- (void) handleDoubleTapOnPad;
- (void) objectWasSelected : (TObject *)object;
- (void) objectWasModifiedByEditor;
- (void) setupObjectInspector;

@end
