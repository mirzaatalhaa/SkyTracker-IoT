export const handler = async (event) => {
    try {
        // Get query params
        const lat = event.queryStringParameters?.lat || "10.1445";
        const lon = event.queryStringParameters?.lon || "76.3943";

        const url = `https://api.airplanes.live/v2/point/${lat}/${lon}/250`;

        const response = await fetch(url);
        const data = await response.json();

        return {
            statusCode: 200,
            headers: {
                "Content-Type": "application/json",
                "Access-Control-Allow-Origin": "*",
            },
            body: JSON.stringify(data),
        };
    } catch (error) {
        console.error("Flights Lambda error:", error);

        return {
            statusCode: 500,
            body: JSON.stringify({
                error: "Failed to fetch flight data",
            }),
        };
    }
};