#ifndef ValhallaWrapperHeader_h
#define ValhallaWrapperHeader_h

#import <Foundation/Foundation.h>

@class ValhallaWrapper;

@interface ValhallaWrapper : NSObject {
    @private
    void* _actor;
}

- (instancetype)initWithConfigPath:(NSString*)config_path error:(__autoreleasing NSError **)error;

- (NSString*)route:(NSString*)request;

- (NSString*)optimizedRoute:(NSString*)request;

- (NSString*)height:(NSString*)request;

- (NSString*)locate:(NSString*)request;

- (NSString*)traceRoute:(NSString*)request;

- (NSString*)matrix:(NSString*)request;

- (NSString*)isochrone:(NSString*)request;

@end

#endif /* ValhallaWrapperHeader_h */
