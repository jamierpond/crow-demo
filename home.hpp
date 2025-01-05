constexpr auto HOME_TEMPLATE_HTML = R"(
<!DOCTYPE html>
<html>
  <head>
    <title>Shorten your link</title>
  </head>
  <body>
    <h1>Shorten your link</h1>
    <form id="shortenForm">
      <label for="link">Link:</label>
      <input type="text" id="link" name="link" required>
      <input type="submit" value="Submit">
    </form>
    <div id="result" hidden>
      <h3>Your shortened link:</h3>
      <a id="shortenedLink"></a>
    </div>

    <script>
      document.getElementById('shortenForm').addEventListener('submit', async (event) => {
        event.preventDefault();

        try {
          const response = await fetch('/insert', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({
              "link": document.getElementById('link').value
            })
          });

          if (!response.ok) {
            console.error('Error:', response);
            throw new Error(response.data);
          }
          const data = await response.json();

          document.getElementById('shortenedLink').textContent = data.short_link;
          document.getElementById('result').hidden = false;
        } catch (error) {
          console.error('Error:', error);
          alert('An error occurred while shortening the link' + error.message);
        }
      });
    </script>
  </body>
</html>
)";

