wrk.method = "POST"
wrk.headers["Content-Type"] = "application/json"
wrk.body = '{ "imageLink": "https://images.pexels.com/photos/5230612/pexels-photo-5230612.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=1", "width": 100, "height": 150, "format": "jpg" }'

function setup(thread)
   thread:set("threadid", thread.id)
end

function init(args)
   requests = 10  -- Number of concurrent requests
   thread_id = 0
end

function request()
   thread_id = thread_id + 1
   local id = thread_id % requests
   local path = "/resize?id=" .. id
   return wrk.format("POST", path, nil, wrk.body)
end

-- Invoker Concurrency Test 
-- wrk -c 10 -t 10 -d 10s -s request.lua http://localhost:80


-- wrk.method = "POST"
-- wrk.headers["Content-Type"] = "application/json"
-- wrk.body = '{ "imageLink": "https://images.pexels.com/photos/5230612/pexels-photo-5230612.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=1", "size": "500x400" }'

