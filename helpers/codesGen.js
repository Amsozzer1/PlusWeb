var fs = require('fs');

function main(){
    let obj = JSON.parse(fs.readFileSync('./codes.json', 'utf8'));
    
    let output = '';
    
    // Iterate through all codes and filter only numeric ones
    for (const [key, value] of Object.entries(obj)) {
        // Only process entries with numeric codes (skip category entries like "1xx", "2xx")
        if (typeof value.code === 'number') {
            output += `{${value.code}, "${value.message}"},\n`;
        }
    }
    
    // Remove the last comma and newline, then add final newline
    output = output.slice(0, -2) + '\n';
    
    // Write to text file
    fs.writeFileSync('./http_codes.txt', output);
    
    console.log('HTTP status codes written to http_codes.txt');
    console.log('Preview:');
    console.log(output.split('\n').slice(0, 10).join('\n') + '\n...');
}

main();