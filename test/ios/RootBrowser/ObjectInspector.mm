//
//  ObjectInspector.m
//  root_browser
//
//  Created by Timur Pocheptsov on 9/6/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "InspectorWithNavigation.h"
#import "FilledAreaInspector.h"
#import "ObjectInspector.h"
#import "LineInspector.h"
#import "PadInspector.h"
#import "EditorView.h"

//C++ (ROOT) imports.
#import "TAttLine.h"
#import "TAttFill.h"
#import "TAttPad.h"
#import "TObject.h"
#import "TClass.h"

@implementation ObjectInspector

//_________________________________________________________________
- (void) initObjectInspectorView
{
   editorView = [[EditorView alloc] initWithFrame:CGRectMake(0.f, 0.f, [EditorView editorWidth], [EditorView editorHeight])];
   self.view = editorView;
   [editorView release];
}

//_________________________________________________________________
- (void) cacheEditors
{
   using namespace ROOT_IOSObjectInspector;

   //TAttLine.
   LineInspector *lineInspectorCompositor = [[LineInspector alloc] initWithNibName : @"LineInspector" bundle : nil];
   InspectorWithNavigation *lineInspector = [[InspectorWithNavigation alloc] initWithRootViewController : lineInspectorCompositor];
   [lineInspectorCompositor release];
   lineInspector.view.frame = [LineInspector inspectorFrame];
   cachedEditors[kAttLine] = lineInspector;
   
   //TAttFill.
   cachedEditors[kAttFill] = [[FilledAreaInspector alloc] initWithNibName : @"FilledAreaInspector" bundle : nil];

   //TAttPad.
   PadInspector *padInspectorCompositor = [[PadInspector alloc] initWithNibName : @"PadInspector" bundle : nil];
   InspectorWithNavigation *padInspector = [[InspectorWithNavigation alloc] initWithRootViewController : padInspectorCompositor];
   [padInspectorCompositor release];
   padInspector.view.frame = [PadInspector inspectorFrame];
   cachedEditors[kAttPad] = padInspector;
}

//_________________________________________________________________
- (id)initWithNibName : (NSString *)nibNameOrNil bundle : (NSBundle *)nibBundleOrNil
{
   self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
   if (self) {
      [self initObjectInspectorView];
      [self cacheEditors];
   }
   return self;
}

//_________________________________________________________________
- (void) dealloc
{
   using ROOT_IOSObjectInspector::kNOfInspectors;

   for (unsigned i = 0; i < kNOfInspectors; ++i)
      [cachedEditors[i] release];

   [super dealloc];
}

//_________________________________________________________________
- (void)didReceiveMemoryWarning
{
   // Releases the view if it doesn't have a superview.
   [super didReceiveMemoryWarning];
   // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

/*
// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView
{
}
*/

/*
// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
    [super viewDidLoad];
}
*/

//_________________________________________________________________
- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

//_________________________________________________________________
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
	return YES;
}

//_________________________________________________________________
- (void) setROOTObjectController : (ROOTObjectController *)c
{
   for (unsigned i = 0; i < nActiveEditors; ++i)
      [activeEditors[i] setROOTObjectController : c];
}

//_________________________________________________________________
- (void) setTitle
{
   if (dynamic_cast<TAttPad *>(object)) {
      //This is special case, as soon as ROOT::iOS::Pad does not have
      //ClassDef, the IsA() will be for TVirtualPad, but I want to
      //see simply "Pad" as a title.
      [editorView setEditorTitle : "Pad"];
   } else {
      [editorView setEditorTitle : object->IsA()->GetName()];
   }
}

//_________________________________________________________________
- (void) setActiveEditors
{
   using namespace ROOT_IOSObjectInspector;

   nActiveEditors = 0;

   if (dynamic_cast<TAttLine *>(object))
      activeEditors[nActiveEditors++] = cachedEditors[kAttLine];
   
   if (dynamic_cast<TAttFill *>(object))
      activeEditors[nActiveEditors++] = cachedEditors[kAttFill];
   
   if (dynamic_cast<TAttPad *>(object))
      activeEditors[nActiveEditors++] = cachedEditors[kAttPad];
}

//_________________________________________________________________
- (void) setROOTObject : (TObject *)o
{
   if (o != object) {
      //Initialize.
      object = o;
      
      [self setTitle];
      [self setActiveEditors];
   
      for (unsigned i = 0; i < nActiveEditors; ++i)
         [activeEditors[i] setROOTObject : o];
      
      [editorView removeAllEditors];

      for (unsigned i = 0; i < nActiveEditors; ++i)
         [editorView addSubEditor : activeEditors[i].view withName : [activeEditors[i] getComponentName]];
   }
}

//_________________________________________________________________
- (void) resetInspector
{
   for (unsigned i = 0; i < nActiveEditors; ++i)
      if ([activeEditors[i] respondsToSelector : @selector(resetInspector)])
         [activeEditors[i] resetInspector];
}

@end
