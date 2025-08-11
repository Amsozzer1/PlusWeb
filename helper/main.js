const express = require('express')
const app = express()


app.get('/', (req,res)=>{
    res.status(200).send({
        "name":"ahmed"
    })
});

app.get("/me",(req,res)=>{
    res.status(200).send({
        "name":"ahmed"
    })
});

app.get("/user/:id",(req,res)=>{
    const userId = req.params.id;
    res.status(200).send({
        "name":"ahmed",
        "id": userId
    })
});

app.listen(3000, () => {
    console.log(`Example app listening on port ${3000}`)
})

