export const handler = async (event) => {
  const apiKey = process.env.AVIATIONSTACK_KEY;
  const COK_IATA = "COK";

  if (!apiKey) {
      return {
          statusCode: 500,
          body: JSON.stringify({
              error: "AVIATIONSTACK_KEY environment variable is not configured",
          }),
      };
  }

  try {
      const [arrRes, depRes] = await Promise.all([
          fetch(`http://api.aviationstack.com/v1/flights?access_key=${apiKey}&arr_iata=${COK_IATA}&limit=100`),
          fetch(`http://api.aviationstack.com/v1/flights?access_key=${apiKey}&dep_iata=${COK_IATA}&limit=100`)
      ]);

      const arrData = await arrRes.json();
      const depData = await depRes.json();

      if (arrData.error || depData.error) {
          return {
              statusCode: 500,
              body: JSON.stringify({
                  error:
                      arrData.error?.message ||
                      depData.error?.message ||
                      "AviationStack API error",
              }),
          };
      }

      return {
          statusCode: 200,
          headers: {
              "Content-Type": "application/json",
              "Access-Control-Allow-Origin": "*",
              "Cache-Control": "max-age=86400"
          },
          body: JSON.stringify({
              arrivals: arrData.data || [],
              departures: depData.data || [],
              lastUpdated: new Date().toISOString(),
          }),
      };
  } catch (error) {
      console.error("Lambda error:", error);

      return {
          statusCode: 500,
          body: JSON.stringify({
              error: "Internal server error while fetching traffic data",
          }),
      };
  }
};