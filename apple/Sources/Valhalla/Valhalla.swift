import ValhallaObjc
import ValhallaModels
import ValhallaConfigModels

public protocol ValhallaProviding {
    
    init(_ config: ValhallaConfig) throws
    
    init(configPath: String) throws

    func route(request: RouteRequest) throws -> RouteResponse

    func optimizedRoute(request: RouteRequest) throws -> RouteResponse

    func height(request: RouteRequest) throws -> RouteResponse

    func locate(request: RouteRequest) throws -> RouteResponse

    func traceRoute(request: MapMatchRequest) throws -> MapMatchRouteResponse

    func matrix(request: MatrixRequest) throws -> MatrixResponse

    func isochrone(request: IsochroneRequest) throws -> IsochroneResponse
}

public final class Valhalla: ValhallaProviding {
    private let actor: ValhallaWrapper?
    private let configPath: String

    public convenience init(_ config: ValhallaConfig) throws {
        let configURL = try ValhallaFileManager.saveConfigTo(config)
        try self.init(configPath: configURL.relativePath)
    }

    public required init(configPath: String) throws {
        do {
            try ValhallaFileManager.injectTzdataIntoLibrary()
        } catch {
            // If you're circumventing this libraries injection, download tzdata.tar and put in your bundle. https://www.iana.org/time-zones
            fatalError("tzdata was not inject into Bundle.main. This can be avoided by including tzdata.tar in your main bundle.")
        }

        self.configPath = configPath
        do {
            self.actor = try ValhallaWrapper(configPath: configPath)
        } catch let error as NSError {
            throw ValhallaError.valhallaError(error.code, error.domain)
        } catch {
            throw ValhallaError.valhallaError(-1, error.localizedDescription)
        }
    }
    
    public func route(request: RouteRequest) throws -> RouteResponse {
        let requestData = try JSONEncoder().encode(request)
        guard let requestStr = String(data: requestData, encoding: .utf8) else {
            throw ValhallaError.encodingNotUtf8("requestStr")
        }
        
        let resultStr = route(rawRequest: requestStr)
        guard let resultData = resultStr.data(using: .utf8) else {
            throw ValhallaError.encodingNotUtf8("resultData")
        }
        
        if let error = try? JSONDecoder().decode(ValhallaErrorModel.self, from: resultData) {
            throw ValhallaError.valhallaError(error.code, error.message)
        }
        
        return try JSONDecoder().decode(RouteResponse.self, from: resultData)
    }

    public func route(rawRequest request: String) -> String {
        actor!.route(request)
    }

    public func optimizedRoute(request: RouteRequest) throws -> RouteResponse {
        let requestData = try JSONEncoder().encode(request)
        guard let requestStr = String(data: requestData, encoding: .utf8) else {
            throw ValhallaError.encodingNotUtf8("requestStr")
        }

        let resultStr = optimizedRoute(rawRequest: requestStr)
        guard let resultData = resultStr.data(using: .utf8) else {
            throw ValhallaError.encodingNotUtf8("resultData")
        }

        if let error = try? JSONDecoder().decode(ValhallaErrorModel.self, from: resultData) {
            throw ValhallaError.valhallaError(error.code, error.message)
        }

        return try JSONDecoder().decode(RouteResponse.self, from: resultData)
    }

    public func optimizedRoute(rawRequest request: String) -> String {
        actor!.optimizedRoute(request)
    }

    public func height(request: RouteRequest) throws -> RouteResponse {
        let requestData = try JSONEncoder().encode(request)
        guard let requestStr = String(data: requestData, encoding: .utf8) else {
            throw ValhallaError.encodingNotUtf8("requestStr")
        }

        let resultStr = height(rawRequest: requestStr)
        guard let resultData = resultStr.data(using: .utf8) else {
            throw ValhallaError.encodingNotUtf8("resultData")
        }

        if let error = try? JSONDecoder().decode(ValhallaErrorModel.self, from: resultData) {
            throw ValhallaError.valhallaError(error.code, error.message)
        }

        return try JSONDecoder().decode(RouteResponse.self, from: resultData)
    }

    public func height(rawRequest request: String) -> String {
        actor!.height(request)
    }

    public func locate(request: RouteRequest) throws -> RouteResponse {
        let requestData = try JSONEncoder().encode(request)
        guard let requestStr = String(data: requestData, encoding: .utf8) else {
            throw ValhallaError.encodingNotUtf8("requestStr")
        }

        let resultStr = locate(rawRequest: requestStr)
        guard let resultData = resultStr.data(using: .utf8) else {
            throw ValhallaError.encodingNotUtf8("resultData")
        }

        if let error = try? JSONDecoder().decode(ValhallaErrorModel.self, from: resultData) {
            throw ValhallaError.valhallaError(error.code, error.message)
        }

        return try JSONDecoder().decode(RouteResponse.self, from: resultData)
    }

    public func locate(rawRequest request: String) -> String {
        actor!.locate(request)
    }

    public func traceRoute(request: MapMatchRequest) throws -> MapMatchRouteResponse {
        let requestData = try JSONEncoder().encode(request)
        guard let requestStr = String(data: requestData, encoding: .utf8) else {
            throw ValhallaError.encodingNotUtf8("requestStr")
        }

        let resultStr = traceRoute(rawRequest: requestStr)
        guard let resultData = resultStr.data(using: .utf8) else {
            throw ValhallaError.encodingNotUtf8("resultData")
        }

        if let error = try? JSONDecoder().decode(ValhallaErrorModel.self, from: resultData) {
            throw ValhallaError.valhallaError(error.code, error.message)
        }

        return try JSONDecoder().decode(MapMatchRouteResponse.self, from: resultData)
    }

    public func traceRoute(rawRequest request: String) -> String {
        actor!.traceRoute(request)
    }

    public func matrix(request: MatrixRequest) throws -> MatrixResponse {
        let requestData = try JSONEncoder().encode(request)
        guard let requestStr = String(data: requestData, encoding: .utf8) else {
            throw ValhallaError.encodingNotUtf8("requestStr")
        }

        let resultStr = matrix(rawRequest: requestStr)
        guard let resultData = resultStr.data(using: .utf8) else {
            throw ValhallaError.encodingNotUtf8("resultData")
        }

        if let error = try? JSONDecoder().decode(ValhallaErrorModel.self, from: resultData) {
            throw ValhallaError.valhallaError(error.code, error.message)
        }

        return try JSONDecoder().decode(MatrixResponse.self, from: resultData)
    }

    public func matrix(rawRequest request: String) -> String {
        actor!.matrix(request)
    }

    public func isochrone(request: IsochroneRequest) throws -> IsochroneResponse {
        let requestData = try JSONEncoder().encode(request)
        guard let requestStr = String(data: requestData, encoding: .utf8) else {
            throw ValhallaError.encodingNotUtf8("requestStr")
        }

        let resultStr = isochrone(rawRequest: requestStr)
        guard let resultData = resultStr.data(using: .utf8) else {
            throw ValhallaError.encodingNotUtf8("resultData")
        }

        if let error = try? JSONDecoder().decode(ValhallaErrorModel.self, from: resultData) {
            throw ValhallaError.valhallaError(error.code, error.message)
        }

        return try JSONDecoder().decode(IsochroneResponse.self, from: resultData)
    }

    public func isochrone(rawRequest request: String) -> String {
        actor!.isochrone(request)
    }
}
